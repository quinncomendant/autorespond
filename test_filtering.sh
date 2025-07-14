#!/bin/bash

# Test script to verify autorespond filtering functionality

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Create a stub qmail-queue if needed
if [[ ! -f /var/qmail/bin/qmail-queue ]]; then
    echo "Creating qmail-queue stub for testing...";
    sudo mkdir -p /var/qmail/bin;
    echo -e '#!/bin/bash\ncat > /tmp/qmail-queue-test.eml' > /var/qmail/bin/qmail-queue;
    chmod +x /var/qmail/bin/qmail-queue;
fi

# Set required environment variables
export SENDER="sender@example.com";
export EXT="recipient";
export HOST="example.net";
export LOCAL="recipient";

# Create temporary logs directory
logs=$(mktemp -d)
echo "Using temporary log directory: $logs";

# Function to run a test case
run_test() {
    local test_name="$1";
    local email_content="$2";
    local should_respond="$3"; # 1 = should respond, 0 = should not respond

    # Clean up previous test output
    rm -f /tmp/qmail-queue-test.eml;

    # Run autorespond with the test email
    echo -e "$email_content" | ./autorespond 3600 5 help_message "$logs" 1 '$' 2>&1 | grep -E "(AUTORESPOND:|exiting)" > /tmp/test_output.txt;

    # Check if a response was generated
    if [[ -f /tmp/qmail-queue-test.eml ]]; then
        response_generated=1;
    else
        response_generated=0;
    fi

    # Check result
    if [[ $response_generated -eq $should_respond ]]; then
        echo -e "${GREEN}✓ $test_name${NC}";
        if [[ $should_respond -eq 0 ]]; then
            echo "  $(cat /tmp/test_output.txt)";
        fi
    else
        echo -e "${RED}✗ $test_name${NC}";
        echo "  Expected: $([ $should_respond -eq 1 ] && echo "response" || echo "no response")";
        echo "  Got: $([ $response_generated -eq 1 ] && echo "response" || echo "no response")";
        echo "  Output: $(cat /tmp/test_output.txt)";
    fi
}

echo -e "\n${YELLOW}=== Testing autorespond filtering functionality ===${NC}\n";

# Test 1: Normal personal email (should respond)
run_test "Normal personal email" \
"Date: $(date -R)
From: John Doe <john@personal-email.com>
To: recipient@example.net
Subject: Hello there

This is a personal email." 1;

# Test 2: Email with List-Id header (should not respond)
run_test "Email with List-Id header" \
"Date: $(date -R)
From: Newsletter <news@company.com>
To: recipient@example.net
Subject: Newsletter
List-Id: Company Newsletter <news.company.com>

Newsletter content." 0;

# Test 3: Email with List-Unsubscribe header (should not respond)
run_test "Email with List-Unsubscribe header" \
"Date: $(date -R)
From: Marketing <marketing@company.com>
To: recipient@example.net
Subject: Special Offer
List-Unsubscribe: <https://company.com/unsubscribe>

Marketing content." 0;

# Test 4: Email with Mailing-List header (should not respond)
run_test "Email with Mailing-List header" \
"Date: $(date -R)
From: Discussion <list@forum.com>
To: recipient@example.net
Subject: Discussion topic
Mailing-List: contact list@forum.com

Discussion content." 0;

# Test 5: Email with X-Patreon-UUID header (should not respond)
run_test "Email with X-Patreon-UUID header" \
"Date: $(date -R)
From: Patreon <bingo@patreon.com>
To: recipient@example.net
Subject: New patron
X-Patreon-UUID: 12345-67890

Patreon notification." 0;

# Test 6: Email with X-Mailgun-Tag header (should not respond)
run_test "Email with X-Mailgun-Tag header" \
"Date: $(date -R)
From: Service <service@app.com>
To: recipient@example.net
Subject: Notification
X-Mailgun-Tag: transactional

Service notification." 0;

# Test 7: Email with Precedence bulk header (should not respond)
run_test "Email with Precedence bulk header" \
"Date: $(date -R)
From: Bulk Sender <bulk@company.com>
To: recipient@example.net
Subject: Bulk message
Precedence: bulk

Bulk content." 0;

# Test 8: Email with X-Spam-Level containing asterisks (should not respond)
run_test "Email with X-Spam-Level containing asterisks" \
"Date: $(date -R)
From: Spammer <spam@spammer.com>
To: recipient@example.net
Subject: Buy now!
X-Spam-Level: ****

Spam content." 0;

# Test 9: Email from noreply address (should not respond)
run_test "Email from noreply address" \
"Date: $(date -R)
From: No Reply <noreply@company.com>
To: recipient@example.net
Subject: Automated message

This is an automated message." 0;

# Test 10: Email from github.com (should not respond)
run_test "Email from github.com" \
"Date: $(date -R)
From: GitHub <notifications@github.com>
To: recipient@example.net
Subject: [Repo] New issue

GitHub notification." 0;

# Test 11: Email with Reply-To matching exception list (should not respond)
run_test "Email with Reply-To matching exception list" \
"Date: $(date -R)
From: Service <service@example.com>
Reply-To: do-not-reply@example.com
To: recipient@example.net
Subject: Service notification

Service message." 0;

# Test 12: Email with Sender header matching exception list (should not respond)
run_test "Email with Sender header matching exception list" \
"Date: $(date -R)
From: Newsletter <news@example.com>
Sender: bounce-12345@amazonses.com
To: recipient@example.net
Subject: Newsletter

Newsletter content." 0;

# Test 13: Email from newsletter@ address (should not respond)
run_test "Email from newsletter@ address" \
"Date: $(date -R)
From: Company Newsletter <newsletter@somecompany.com>
To: recipient@example.net
Subject: Weekly Update

Newsletter content." 0;

# Test 14: Email from mailer-daemon (should not respond)
export SENDER="mailer-daemon@example.com";
run_test "Email from mailer-daemon" \
"Date: $(date -R)
From: Mail Delivery System <mailer-daemon@example.com>
To: recipient@example.net
Subject: Delivery Status Notification

Bounce message." 0;
export SENDER="sender@example.com";

# Test 15: Email with empty sender (should not respond)
export SENDER="";
run_test "Email with empty sender" \
"Date: $(date -R)
From: System <system@example.com>
To: recipient@example.net
Subject: System message

System content." 0;
export SENDER="sender@example.com";

# Test 16: Email from personal address (should respond)
run_test "Email from personal address with company domain" \
"Date: $(date -R)
From: Alice Smith <alice.smith@personalcompany.com>
To: recipient@example.net
Subject: Question about project

I have a question about the project." 1;

# Test 17: Email with Return-Path matching exception list (should not respond)
run_test "Email with Return-Path matching exception list" \
"Date: $(date -R)
From: Service <service@example.com>
Return-Path: <bounce@mailgun.net>
To: recipient@example.net
Subject: Service update

Service update message." 0;

# Test 18: Mixed case List-ID header (should not respond)
run_test "Mixed case List-ID header" \
"Date: $(date -R)
From: Newsletter <news@company.com>
To: recipient@example.net
Subject: Newsletter
LiSt-ID: Company Newsletter <news.company.com>

Newsletter content." 0;

# Test 19: Multiple spam indicators (should not respond)
run_test "Multiple spam indicators" \
"Date: $(date -R)
From: Marketing <marketing@company.com>
To: recipient@example.net
Subject: Special Offer
List-Unsubscribe: <https://company.com/unsubscribe>
Precedence: bulk
X-Spam-Level: ***

Marketing content with multiple spam indicators." 0;

# Test 20: Email from service@domain (should not respond)
run_test "Email from service@ address" \
"Date: $(date -R)
From: Customer Service <service@somecompany.com>
To: recipient@example.net
Subject: Your ticket has been updated

Service notification." 0;

# Test 21: Email from offers@ address (should not respond)
run_test "Email from offers@ address" \
"Date: $(date -R)
From: Special Offers <offers@store.com>
To: recipient@example.net
Subject: 50% off sale!

Marketing email." 0;

# Test 22: Email from sales@ address (should not respond)
run_test "Email from sales@ address" \
"Date: $(date -R)
From: Sales Team <sales@company.com>
To: recipient@example.net
Subject: Follow up on your inquiry

Sales email." 0;

# Test 23: Email from alert@ address (should not respond)
run_test "Email from alert@ address" \
"Date: $(date -R)
From: System Alert <alert@monitoring.com>
To: recipient@example.net
Subject: Server CPU usage high

Alert notification." 0;

# Test 24: Email from help@ address (should not respond)
run_test "Email from help@ address" \
"Date: $(date -R)
From: Help Desk <help@support.com>
To: recipient@example.net
Subject: Ticket resolved

Support notification." 0;

# Test 25: Email from announcement@ address (should not respond)
run_test "Email from announcement@ address" \
"Date: $(date -R)
From: Company Announcements <announcement@company.com>
To: recipient@example.net
Subject: New feature release

Announcement email." 0;

# Test 26: Email with do-not-reply variant (should not respond)
run_test "Email from do-not-reply variant" \
"Date: $(date -R)
From: System <do_not_reply@company.com>
To: recipient@example.net
Subject: Account update

Automated message." 0;

# Test 27: Email with donotreply variant (should not respond)
run_test "Email from donotreply variant" \
"Date: $(date -R)
From: Automated System <donotreply@service.com>
To: recipient@example.net
Subject: Password reset

Automated notification." 0;

# Test 28: Email from bounce address (should not respond)
run_test "Email from bounce address" \
"Date: $(date -R)
From: Mail System <bounce-12345@mailservice.com>
To: recipient@example.net
Subject: Delivery notification

Bounce notification." 0;

# Test 29: Email with extra spaces in headers (should not respond)
run_test "Headers with extra whitespace" \
"Date: $(date -R)
From: Newsletter <news@company.com>
To: recipient@example.net
Subject: Newsletter
List-Id:   Company Newsletter <news.company.com>

Content with extra spaces in headers." 0;

# Test 30: Email from Twitter/X.com (should not respond)
run_test "Email from x.com" \
"Date: $(date -R)
From: X <notify@x.com>
To: recipient@example.net
Subject: New follower

Social media notification." 0;

# Test 31: Email from LinkedIn (should not respond)
run_test "Email from LinkedIn" \
"Date: $(date -R)
From: LinkedIn <messages-noreply@linkedin.com>
To: recipient@example.net
Subject: You have a new message

LinkedIn notification." 0;

# Test 32: Email from Zoom (should not respond)
run_test "Email from Zoom" \
"Date: $(date -R)
From: Zoom <no-reply@zoom.us>
To: recipient@example.net
Subject: Meeting reminder

Zoom notification." 0;

# Test 33: Email with Precedence list (should not respond)
run_test "Email with Precedence list" \
"Date: $(date -R)
From: Discussion Forum <forum@example.com>
To: recipient@example.net
Subject: New post in thread
Precedence: list

Forum notification." 0;

# Test 34: Email with Precedence junk (should not respond)
run_test "Email with Precedence junk" \
"Date: $(date -R)
From: Promotions <promo@example.com>
To: recipient@example.net
Subject: Limited time offer
Precedence: junk

Promotional content." 0;

# Test 35: Personal email with similar domain (should respond)
run_test "Personal email from githubuser.com" \
"Date: $(date -R)
From: John Doe <john@githubuser.com>
To: recipient@example.net
Subject: Question about project

I have a question about the project." 1;

# Test 36: Personal email from .org domain (should respond)
run_test "Personal email from .org domain" \
"Date: $(date -R)
From: Alice Smith <alice@personalsite.org>
To: recipient@example.net
Subject: Meeting tomorrow

Can we meet tomorrow?" 1;

# Test 37: Email with Return-Path bounce address (should not respond)
run_test "Return-Path with bounce pattern" \
"Date: $(date -R)
From: Service <service@example.com>
Return-Path: <bounce-abc123@example.com>
To: recipient@example.net
Subject: Service update

Service message." 0;

# Test 38: Email from subdomain of blocked domain (should not respond)
run_test "Email from subdomain of github.com" \
"Date: $(date -R)
From: GitHub Enterprise <noreply@enterprise.github.com>
To: recipient@example.net
Subject: Repository update

GitHub notification." 0;

# Test 39: Email with no X-Spam-Level asterisks (should respond)
run_test "Low spam score" \
"Date: $(date -R)
From: Fred <fred@workemail.com>
To: recipient@example.net
Subject: Boring work email
X-Spam-Level: 

Not a very spammy message." 1;

# Test 40: This envelope sender has already received too many auto-replies within the specified time frame (should not respond).
run_test "Threshold exceeded from sender@example.com" \
"Date: $(date -R)
From: Bob Johnson <bob.johnson@personalmail.com>
To: recipient@example.net
Subject: Sorry for sending you so many emails today

Note it is the envelope sender address that is counted, not From address." 0;

# Test 41: Email from mailer-daemon variant (should not respond)
export SENDER="MAILER-DAEMON@example.com";
run_test "Email from MAILER-DAEMON (uppercase)" \
"Date: $(date -R)
From: Mail Delivery System <MAILER-DAEMON@example.com>
To: recipient@example.net
Subject: Undelivered Mail Returned to Sender

Bounce message." 0;
export SENDER="sender@example.com";

# Test 42: Email with sender #@[] (should not respond)
export SENDER="#@[]";
run_test "Email with special sender #@[]" \
"Date: $(date -R)
From: System <system@example.com>
To: recipient@example.net
Subject: System message

System content." 0;
export SENDER="sender@example.com";

# Test 43: Email with a X-Report-Abuse-To header.
run_test "Email with a X-Report-Abuse-To header." \
"Date: $(date -R)
From: transactional@example.com
To: recipient@example.net
X-Report-Abuse-To: abuse@example.com
Subject: This is probably a transactional email

Beep boop." 0;

# Test 44: Email sent from the mailx client
run_test "Email with a User-Agent: Heirloom mailx" \
"Date: $(date -R)
From: user@server.example.com
To: user@example.net
User-Agent: Heirloom mailx
Subject: perhaps from a cron script

Output from, e.g., crond. " 0;

# Clean up
rm -rf "$logs";
rm -f /tmp/test_output.txt;
rm -f /tmp/qmail-queue-test.eml;

echo -e "\n${YELLOW}=== Test completed ===${NC}";
