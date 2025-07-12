#!/bin/bash

if [[ ! -f /var/qmail/bin/qmail-queue ]]; then
    echo "/var/qmail/bin/qmail-queue not found. Creating a stub for testing…";
    sudo mkdir -p /var/qmail/bin;
    echo -e '#!/bin/bash\ncat > /tmp/qmail-queue-test.eml' > /var/qmail/bin/qmail-queue;
    chmod +x /var/qmail/bin/qmail-queue;
fi

# Set required environment variables.
export SENDER="sender@example.com"; # The envelope sender address of the message.
export EXT="recipient"; # For autorepond, this is the local recipientname. For qmail, this is the portion of the local part of the recipient address following the first dash.
export HOST="example.net"; # The domain part of the recipient address.
export LOCAL="recipient"; # The local part of the recipient address.

# Create temporary logs directory
logs=$(mktemp -d)

echo -e "=== Testing autorespond template parsing ===\nExecuting ./autorespond 3600 5 help_message '$logs' 1 '\$'";

# Generate test email and pipe directly to autorespond;
echo "From: sender@example.com
To: recipient-ext@example.net
Subject: Test message
Date: $(date -R)

This is a test email to check autorespond template functionality.
" | ./autorespond 3600 5 help_message "$logs" 1 '$';

if [[ ! -s /tmp/qmail-queue-test.eml ]]; then
    echo "Test failed: /tmp/qmail-queue-test.eml is empty";
    exit 1;
fi

echo "Test completed. Here's the output from /tmp/qmail-queue-test.eml:";
echo "-----------------------------------------------------------------";
cat /tmp/qmail-queue-test.eml;
echo "-----------------------------------------------------------------";
echo "It should show:
- From: From: Support <help@company.com>
- Subject: Help Response
- Body should contain template message without duplicate headers";
