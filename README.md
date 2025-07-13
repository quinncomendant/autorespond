# autorespond

Simple autoresponder for `qmail`.

==============================================

Mail is sent to help@my-company.com. An automatically generated response
is sent back to the user with an address of "help@my-company.com".  You
can set the envelope sender to an empty string.  However, some programs
will parse the message for the "From:"  field and send an autoresponse
back to it.  It is received at your autoresponder, and you now have a mail
loop. 

This autoresponder also catches some other simple situations such as mail
from a mailer-daemon, empty envelope sender, bulk precedence headers, etc. 


## Install

First thing, if your qmail distribution is not in /var/qmail, then edit
autorespond.c and change QMAIL_LOCATION to point to the correct directory. 

If you do not want to make use of the 2 optional commandline options edit 
the 2 defines:

- DEFAULT_MH    -> corresponds to the "flag" optional parameter
- DEFAULT_FROM  -> connesponds to the "arsender" optional parameter

There shouldn't be anything else to configure.  Compile: 

```
make
make install
```

## Usage

Usage is as follows:

``autorespond time num message dir [ flag arsender ]``

     time - amount of time to consider a message (in seconds)
     num - maximum number of messages to allow within time seconds
     message - the filename of the message to send
     dir - the directory to hold the log of messages

     optional parameters:

     flag - handling of original message:

       0 - append nothing
       1 - append quoted original message without attachments <default>

     arsender - from address in generated message, or:

       + = blank from envelope (empty sender)
       $ = recipient address will be used (EXT@HOST from qmail environment) <default>
       any other value = used as-is as the from address


For example, in your ~alias directory, create a .qmail-help file with
the following:

```
|autorespond 10000 5 help_message help_autorespond 1
&your-email-address@company.com
```

This will allow up to five messages from the sender within about 3 hours 
and include the original message as a quote.

```
|autorespond 10000 10 help_message help_autorespond 0 +
&your-email-address@company.com
```

This will allow up to ten messages from the sender within about 3 hours and
will NOT include the original message as a quote and will leave the from
field blank.

Create a file in your ~alias directory called help_message.  It should say
something like this: 
```
From: Support <help@company.com>
Subject: Help Response

This is a response to your help request.


Below is a copy of the message we received.
```
--------

Make sure that "From:" is the first line of the file.  You should probably
include the dashes to separate your help message from the user's message. 
An example is included with this package. 

Create a directory called help_autorespond in your ~alias directory.  This
is where the log of messages goes. 

That should be it.

## Notes
9/18/2003
- If the maximum count has been reached, the autoresponse doesn't 
  get shipped out, but the message is still processed as usual.
  This is a change from older versions because it didn't make
  sense before.

## More info and support
To find more info and ask for support post a comment in [my blog](https://notes.sagredo.eu/en/qmail-notes-185/autorespond-24.html).
