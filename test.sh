#!/bin/bash

green='\033[0;32m';
red='\033[0;31m';
reset='\033[0m';

_info() {
    local IFS='';
    echo -e "${green}$*${reset}";
}

_error() {
    local IFS='';
    echo -e "${red}$*${reset}" >&2;
    exit 1;
}

if [[ ! -f /var/qmail/bin/qmail-queue ]]; then
    _info "/var/qmail/bin/qmail-queue not found. Creating a stub for testing…";
    sudo mkdir -p /var/qmail/bin;
    echo -e '#!/bin/bash\ncat > /tmp/qmail-queue-test.eml' | sudo tee /var/qmail/bin/qmail-queue;
    sudo chmod +x /var/qmail/bin/qmail-queue;
fi

to="sender@example.com"
while getopts "t:" opt; do
    case "$opt" in
        t) to="$OPTARG" ;;
        *) _error "Invalid option: -$OPTARG" ;;
    esac
done

# Set required environment variables.
export SENDER=$to; # The envelope sender address of the message.
export EXT="recipient"; # For autorepond, this is the local recipientname. For qmail, this is the portion of the local part of the recipient address following the first dash.
export HOST="example.net"; # The domain part of the recipient address.
export LOCAL="recipient"; # The local part of the recipient address.

# Create temporary logs directory
logs=$(mktemp -d);

_info "=== Testing autorespond template parsing ===\nExecuting ./autorespond 3600 5 help_message '$logs' 1 '\$'";

# Generate test email and pipe directly to autorespond;
./autorespond 3600 5 help_message "$logs" 1 '$' <<< "Date: $(date -R)
From: Synthetic Sender <sender@example.com>
Reply-To: Lorem/ipsum <reply+ABUQXNB4LLBDEUCPIEGJBSWGO3W2XEVBNHHLCV4RU4@reply.example.com>
To: Synthetic Recipient <recipient-ext@example.net>
Cc: Quinn Comendant <quinn@example.com>,
  Manual <manual@noreply.example.com>
Subject: Re: [Lorem/ipsum] [Bug]: Error: Lorem ipsum dolor sit amet,
 consectetur adipisicing elit, sed do eiusmod tempor incididunt ut
 veniam. (Issue #7159)
Message-ID: <Lorem/ipsum/7159/3043151119@example.com>
In-Reply-To: <Lorem/ipsum/7159@example.com>
References: <Lorem/ipsum/7159@example.com>

This is a test email to check autorespond template functionality.
";

if [[ ! -f /tmp/qmail-queue-test.eml ]] && ! grep -q /tmp/qmail-queue-test.eml /var/qmail/bin/qmail-queue; then
    _info "Test message sent. Please check your mail queue or mailbox at ${to}";
    exit 0;
fi

if [[ ! -s /tmp/qmail-queue-test.eml ]]; then
    _error "Test failed: /tmp/qmail-queue-test.eml is empty";
fi

_info "Test completed. Here's the output from /tmp/qmail-queue-test.eml:";
_info "-----------------------------------------------------------------";
cat /tmp/qmail-queue-test.eml;
_info "-----------------------------------------------------------------";
_info "It should show:
- From: From: Support <help@company.com>
- Subject: Help Response
- Body should contain template message without duplicate headers";

# Cleanup temporary files.
rm -f /tmp/qmail-queue-test.eml;
