# Autorespond Filtering Implementation

## Overview

This document describes the filtering functionality added to autorespond to prevent it from sending automatic replies to non-personal emails such as spam, newsletters, transactional emails, and other automated messages.

## Implementation Details

### New Features Added

1. **Extended Header Checking**
   - `List-Id` - Blocks mailing list messages
   - `List-Unsubscribe` - Blocks marketing emails and newsletters
   - `X-Patreon-UUID` - Blocks Patreon notifications
   - `X-Mailgun-Tag` - Blocks emails sent via Mailgun
   - `X-Spam-Level` - Blocks emails with spam indicators (asterisks)

2. **Sender Pattern Matching**
   - Added regex-based pattern matching for common non-personal email patterns
   - Checks `From`, `Reply-To`, `Sender`, and `Return-Path` headers
   - Blocks emails from addresses matching patterns like:
     - `noreply@`, `no-reply@`, `do-not-reply@`, `donotreply@`
     - `bounce@`, `bounces@`, `bounce-*@`
     - `alert@`, `alerts@`
     - `help@`, `service@`, `support@`
     - `offers@`, `sales@`, `marketing@`
     - `newsletter@`, `newsletters@`
     - `announcement@`, `announcements@`

3. **Domain Blocking**
   - Comprehensive list of 200+ domains known to send automated emails
   - Includes social media, e-commerce, streaming services, email service providers
   - Examples: github.com, linkedin.com, amazon.com, netflix.com, etc.

### Code Changes

1. **Added includes and defines** (lines 78-94):
   ```c
   #include <regex.h>
   #define SENDER_EXCEPTION_LIST "..."
   ```

2. **Added regex matching function** (lines 548-562):
   ```c
   int regex_matches_header(const char *header_str)
   ```

3. **Added header checks in main()** (lines 686-754):
   - Check for presence of blocking headers
   - Check X-Spam-Level for asterisks
   - Check sender headers against exception patterns

### How It Works

1. When an email is received, autorespond reads all headers
2. It checks for the presence of specific headers that indicate automated emails
3. It uses regex pattern matching to check if sender addresses match known patterns
4. If any check matches, the program exits with code 0 (success) without sending a reply
5. Only emails that pass all checks receive an automatic response

### Testing

Two test scripts are included:

1. **test_filtering.sh** - Basic functionality tests
   - Tests each type of header blocking
   - Tests pattern matching for different sender formats
   - Verifies personal emails still receive responses

2. **test_edge_cases.sh** - Comprehensive edge case tests
   - Mixed case headers
   - Multiple spam indicators
   - Subdomain matching
   - Various email address formats

Run tests with:
```bash
chmod +x test_filtering.sh test_edge_cases.sh
./test_filtering.sh
./test_edge_cases.sh
```

### Configuration

The filtering is aggressive by design - it's better to miss sending an auto-reply than to send one to an automated system. The current configuration:

- Case-insensitive header matching
- Supports email addresses in both plain format and angle bracket format
- Blocks subdomains of listed domains
- No configuration file needed - all patterns are compiled into the binary

### Exit Codes

The implementation maintains the existing exit code behavior:
- `0` - Message filtered, no response sent (success)
- `100` - Hard error (e.g., mail loop detected)
- `111` - Soft error (e.g., file access issues)

### Performance Impact

- Minimal performance impact due to efficient regex compilation
- Regex is compiled once per execution
- Header inspection uses existing header parsing infrastructure
- No external dependencies or file I/O for filtering decisions

### Security Considerations

- All patterns are hardcoded to prevent injection attacks
- Regex patterns are carefully escaped to prevent regex DoS
- No user input is used in constructing the regex patterns
- Maintains existing security practices of the original code

### Future Enhancements

Possible improvements for future versions:
1. Configuration file for custom patterns
2. Whitelist functionality for specific senders
3. Logging of filtered messages for statistics
4. Rate limiting based on sender patterns
5. Machine learning-based classification