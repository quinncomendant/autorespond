/*
	autorespond for qmail

	Copyright 1998 Eric Huss
	E-mail: e-huss@netmeridian.com

	Patched 2000 by Matthias Henze <matthias@mhcsoftware.de>
		Patched 2001 by Brad Dameron <bdameron@tscnet.com>
		Patched 2016 by Joakim Karlsson <joakim@roffe.nu>
		Patched 2024 by Roberto Puzzanghera <roberto dot puzzanghera at sagredo dot eu>

	Usage:

			autorespond time num message dir [ flag arsender ]

		time - time in seconds to consider consecutive
		num - maximum number of messages to respond to within time seconds
		message - the message file
		dir - directory to store list of e-mail addresses

		optional parameters:
			flag - heandling of original message:
			0 - append nothing
			1 - append quoted original message without atatchments

		arsender - from adress in generated message (* = None)

		!! edit DEFAULT_MH and DEFAULT_FROM below !!

	CHANGELOG:
		MH 07/2000	1.1.0	added message handling
		MH 07/2000	1.1.0	added from adrees commandline option
		MH 07/2000	1.1.0a	changed back to 1.0.0 compatibility
					the new commandline options are optional by now
		BD 06/2001	2.0.0   Removed excess code, cleaned up some code
		JKA 04/2016     2.0.6	Fixed Message-ID to comply with RFC
				RP 07/2024      Fixed several compilation warnings. Destination dir is now /usr/local

		MH - Matthias Henze <matthias@mhcsoftware.de>
		BD - Brad Dameron <bdameron@tscnet.com>
		JKA - Joakim Karlsson <joakim@roffe.nu>
		RP - Roberto Puzzanghera <roberto dot puzzanghera at sagredo dot eu>

	TODO:

	- clean up 
	- better error handling (e.g. filecreation)

*/

/*
Exit codes:
0 - OK
99 - OK...stop processing lines in .qmail
100 - hard error
111 - soft error
*/

/*
SENDER - envelope sender
NEWSENDER - forwarding evelope sender
RECIPIENT - envelope "to"
USER - the user
HOME - your home directory
HOST - host of recipient address
LOCAL - local of recipient address
EXT - .qmail extension...etc.
DTLINE - Delivered-To
RPLINE - Return-Path
*/

/*Change this value here to the location of your qmail*/
#define QMAIL_LOCATION "/var/qmail"

#include <time.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <regex.h>
#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define DEFAULT_MH	1	/* default value for message_handling flag */
#define DEFAULT_FROM	"$"	/* default "from" for the autorespond */

#define WITH_OMESSAGE	1

#define SENDER_FILTER_LIST "(accounting.*@|billing.*@|bounce.*@|do.?not.?reply.*@|no.?reply.*@|alert.*@|help.*@|service.*@|offers.*@|sales.*@|newsletter.*@|announcement.*@)|[@\\.](abcnews\\.go\\.com|activecampaign\\.com|acxiom\\.com|airbnb\\.com|aliexpress\\.com|amazon\\.com|amazonses\\.com|americanexpress\\.com|apnews\\.com|atlassian\\.com|audible\\.com|aweber\\.com|bankofamerica\\.com|bbc\\.com|beehiiv\\.com|benchmark\\.email|bestbuy\\.com|bitbucket\\.org|bluesky\\.app|booking\\.com|bostonglobe\\.com|bronto\\.com|bsky\\.app|buttondown\\.email|campaignmonitor\\.com|cashapp\\.com|cbsnews\\.com|chase\\.com|cheetahmail\\.com|chicagotribune\\.com|circleci\\.com|clubhouse\\.com|cnn\\.com|codecov\\.io|constantcontact\\.com|convertkit\\.com|crisp\\.chat|deezer\\.com|desk\\.com|discord\\.com|discoursemail\\.com|discoveryplus\\.com|disneyplus\\.com|docker\\.com|drift\\.com|drip\\.com|ebay\\.com|edx\\.org|elasticemail\\.com|eloqua\\.com|emailoctopus\\.com|emarsys\\.com|epsilon\\.com|etsy\\.com|exacttarget\\.com|expedia\\.com|experian\\.com|facebook\\.com|facebookmail\\.com|flickr\\.com|foxnews\\.com|freshdesk\\.com|freshworks\\.com|getresponse\\.com|ghost\\.org|github\\.com|gitlab\\.com|google\\.com|groove\\.co|gumroad\\.com|hbomax\\.com|helpscout\\.com|helpshift\\.com|hilton\\.com|homedepot\\.com|hotels\\.com|hubspot\\.com|hulu\\.com|instagram\\.com|intercom\\.com|iterable\\.com|jenkins\\.io|kayak\\.com|kayako\\.com|kik\\.com|klaviyo\\.com|latimes\\.com|line\\.me|linkedin\\.com|listrak\\.com|livechat\\.com|lyft\\.com|mailchimpapp\\.com|mailerlite\\.com|mailersend\\.com|mailgun\\.net|mailjet\\.com|mandrill\\.com|marketo\\.com|marriott\\.com|mastercard\\.com|mastodon\\.social|mautic\\.org|medium\\.com|meetup\\.com|mlsend\\.com|moosend\\.com|nbcnews\\.com|netflix\\.com|newegg\\.com|nextdoor\\.com|npmjs\\.com|npr\\.org|nypost\\.com|nytimes\\.com|olark\\.com|omnisend\\.com|pandora\\.com|paramountplus\\.com|pardot\\.com|patreon\\.com|paypal\\.com|peacocktv\\.com|pepipost\\.com|phplist\\.com|pinterest\\.com|politico\\.com|postmark\\.com|postmarkapp\\.com|primevideo\\.com|quickbooks\\.intuit\\.com|reddit\\.com|responsys\\.com|reuters\\.com|revue\\.getrevue\\.co|sailthru\\.com|salesforce\\.com|sendfox\\.com|sendgrid\\.net|sendinblue\\.com|sendpulse\\.com|sendwithus\\.com|sendy\\.co|sfgate\\.com|shopify\\.com|signal\\.org|silverpop\\.com|skype\\.com|skyscanner\\.net|slack\\.com|smtp\\.com|snapchat\\.com|socketlabs\\.com|sparkpost\\.com|spotify\\.com|squareup\\.com|stackoverflow\\.com|stripe\\.com|substack\\.com|target\\.com|tawk\\.to|telegram\\.org|theguardian\\.com|threads\\.net|tiktok\\.com|tinyletter\\.com|tripadvisor\\.com|trivago\\.com|tumblr\\.com|turbosmtp\\.com|twitch\\.tv|twitter\\.com|uber\\.com|usatoday\\.com|uservoice\\.com|venmo\\.com|viber\\.com|vimeo\\.com|visa\\.com|walmart\\.com|washingtonpost\\.com|wayfair\\.com|wechat\\.com|wellsfargo\\.com|whatsapp\\.com|wsj\\.com|x\\.com|yesmail\\.com|youtube\\.com|zellepay\\.com|zendesk\\.com|zoom\\.us|zopim\\.com)(>|$)"

#define HR_BUFFER_SIZE 1024

typedef struct _headers {
	char *tag;
	char *content;
	struct _headers *next;
} headers;

headers *header = (headers *)NULL;



/*see header file for more info*/

static char *binqqargs[2] = { "bin/qmail-queue", 0 };
static char *montab[12] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };

/* Function prototypes */
void * safe_malloc(size_t size);
void * safe_realloc(void * ptr, size_t size);
char * read_file(char * filename);
int validate_directory_path(const char *path);
int validate_email_address(const char *email);
int create_secure_temp_file(char *filename_buf, size_t buf_size, const char *prefix);
char* sanitize_header_content(const char* content);
int validate_header_tag(const char* tag);

/****************************************************************/

/* Validate directory path to prevent directory traversal attacks */
int validate_directory_path(const char *path) {
    if (!path || strlen(path) == 0) {
        return 0;
    }
    
    /* Check for directory traversal attempts */
    if (strstr(path, "../") || strstr(path, "..\\") || strcmp(path, "..") == 0) {
        return 0;
    }
    
    /* Check for other dangerous patterns */
    if (strchr(path, '\0') != path + strlen(path)) {
        return 0;  /* embedded null bytes */
    }
    
    /* Check for excessive length */
    if (strlen(path) > PATH_MAX) {
        return 0;
    }
    
    return 1;  /* path is safe */
}

/* Validate email address format */
int validate_email_address(const char *email) {
    const char *at_pos;
    
    if (!email || strlen(email) == 0) {
        return 0;
    }
    
    /* Basic email validation */
    at_pos = strchr(email, '@');
    if (!at_pos || at_pos == email || at_pos == email + strlen(email) - 1) {
        return 0;
    }
    
    /* Check for dangerous characters */
    if (strchr(email, '\n') || strchr(email, '\r') || strchr(email, '\0') != email + strlen(email)) {
        return 0;
    }
    
    return 1;
}

/* Create secure temporary file with random name */
int create_secure_temp_file(char *filename_buf, size_t buf_size, const char *prefix) {
    int fd;
    int attempts = 0;
    unsigned int rand_suffix;
    
    do {
        /* Generate random suffix */
        rand_suffix = (unsigned int)random();
        
        /* Create filename with random component */
        snprintf(filename_buf, buf_size, "%s%u.%u.%u", 
                prefix, getpid(), (unsigned int)time(NULL), rand_suffix);
        
        /* Try to create file exclusively */
        fd = open(filename_buf, O_CREAT | O_EXCL | O_WRONLY, 0600);
        attempts++;
        
    } while (fd == -1 && errno == EEXIST && attempts < 100);
    
    return fd;
}

/* Sanitize header content to prevent injection attacks */
char* sanitize_header_content(const char* content) {
    size_t len;
    char* sanitized;
    size_t j = 0;
    size_t i;
    
    if (!content) return NULL;
    
    len = strlen(content);
    if (len > 8192) {  /* Limit header length */
        return NULL;
    }
    
    sanitized = (char*)safe_malloc(len + 1);
    
    for (i = 0; i < len; i++) {
        unsigned char c = content[i];
        
        /* Remove control characters except tab, CR, LF */
        if (c < 32 && c != '\t' && c != '\r' && c != '\n') {
            continue;
        }
        
        /* Remove DEL character */
        if (c == 127) {
            continue;
        }
        
        /* Prevent header injection */
        if (c == '\n' || c == '\r') {
            /* Only allow if followed by whitespace (header continuation) */
            if (i + 1 < len && (content[i + 1] == ' ' || content[i + 1] == '\t')) {
                sanitized[j++] = c;
            }
        } else {
            sanitized[j++] = c;
        }
    }
    
    sanitized[j] = '\0';
    return sanitized;
}

/* Validate header tag format */
int validate_header_tag(const char* tag) {
    const char* p;
    
    if (!tag || strlen(tag) == 0) {
        return 0;
    }
    
    /* Check length limit */
    if (strlen(tag) > 256) {
        return 0;
    }
    
    /* Header tags should only contain printable ASCII characters */
    for (p = tag; *p; p++) {
        if (*p < 33 || *p > 126 || *p == ':') {
            return 0;
        }
    }
    
    return 1;
}

/****************************************************************/
void * safe_malloc(size_t size)
{
void * ptr;

	ptr = malloc(size);
	if(ptr==NULL) {
		/*exit...no memory*/
		_exit(111);
	}

	return ptr;
}


/****************************************************************/
void * safe_realloc(void * ptr, size_t size)
{
void * p;

	p = realloc(ptr,size);
	if(p==NULL) {
		/*exit...no memory*/
		_exit(111);
	}

	return p;
}

/****************************************************************/
char * read_file(char * filename)
{
FILE * f;
unsigned long size;
char * frb;

	if(!filename) {
		/*failed*/
		return NULL;
	}

	f = fopen(filename,"rb");
	if(f==NULL) {
		/*failed*/
		return NULL;
	}

	/*seek to end*/
	if(fseek(f,0,SEEK_END)==-1) {
		/*failed*/
		fclose(f);
		return NULL;
	}

	/*this may not be portable (although it almost always is)*/
	{
		long file_size = ftell(f);
		if(file_size == -1) {
			fclose(f);
			return NULL;
		}
		
		/* Check for potential overflow */
		if(file_size > ULONG_MAX - 100) {
			fclose(f);
			return NULL;
		}
		
		size = (unsigned long)file_size;
	}

	/*return to the beginning*/
	if(fseek(f,0,SEEK_SET)==-1) {
		fclose(f);
		return NULL;
	}

	frb = (char *) safe_malloc(size+100);

	if(fread(frb,1,size,f)!=size) {
		fclose(f);
		return NULL;
	}
	frb[size]=0;					/*for safety*/
	fclose(f);
	return frb;
}


/****************************************************************
** A wrapper for qmail-queue
** borrowed from djb                   */

int send_message(char * msg, char * from, char ** recipients, int num_recipients)
{
/*	...Adds Date:
	...Adds Message-Id:*/
int r;
int wstat;
int i;
struct tm * dt;
unsigned long msgwhen;
FILE * fdm;
FILE * fde;
pid_t pid;
int pim[2];				/*message pipe*/
int pie[2];				/*envelope pipe*/
FILE *mfp;
char msg_buffer[256];

	/*open a pipe to qmail-queue*/
	if(pipe(pim)==-1 || pipe(pie)==-1) {
		fprintf(stderr, "AUTORESPOND: Reply failed to send from %s to %s: %s.\n", from, recipients[0], strerror(errno));
		return -1;
	}
	pid = vfork();
	if(pid == -1) {
		/*failure*/
		fprintf(stderr, "AUTORESPOND: Reply failed to send from %s to %s: vfork failed - %s.\n", from, recipients[0], strerror(errno));
		return -1;
	}
	if(pid == 0) {
		/*I am the child*/
		close(pim[1]);
		close(pie[1]);
		/*switch the pipes to fd 0 and 1
		  pim[0] goes to 0 (stdin)...the message*/
		if(fcntl(pim[0],F_GETFL,0) == -1) {
			fprintf(stderr, "AUTORESPOND: Reply failed to send from %s to %s: Failed to get status flags for message pipe.\n", from, recipients[0]);
			_exit(120);
		}
		close(0);
		if(fcntl(pim[0],F_DUPFD,0)==-1) {
			fprintf(stderr, "AUTORESPOND: Reply failed to send from %s to %s: Failed to duplicate message pipe descriptor.\n", from, recipients[0]);
			_exit(120);
		}
		close(pim[0]);
		/*pie[0] goes to 1 (stdout)*/
		if(fcntl(pie[0],F_GETFL,0) == -1) {
			fprintf(stderr, "AUTORESPOND: Reply failed to send from %s to %s: Failed to get status flags for envelope pipe.\n", from, recipients[0]);
			_exit(120);
		}
		close(1);
		if(fcntl(pie[0],F_DUPFD,1)==-1) {
			fprintf(stderr, "AUTORESPOND: Reply failed to send from %s to %s: Failed to duplicate envelope pipe descriptor.\n", from, recipients[0]);
			_exit(120);
		}
		close(pie[0]);
		if(chdir(QMAIL_LOCATION) == -1) {
			fprintf(stderr, "AUTORESPOND: Reply failed to send from %s to %s: Failed to change to qmail directory.\n", from, recipients[0]);
			_exit(120);
		}
		execv(*binqqargs,binqqargs);
		_exit(120);
	}

	/*I am the parent*/
	fdm = fdopen(pim[1],"wb");					/*updating*/
	fde = fdopen(pie[1],"wb");
	if(fdm==NULL || fde==NULL) {
		fprintf(stderr, "AUTORESPOND: Reply failed to send from %s to %s: Failed to open pipe streams.\n", from, recipients[0]);
		return -1;
	}
	close(pim[0]);
	close(pie[0]);

	/*prepare to add date and message-id*/
	msgwhen = time(NULL);
	dt = gmtime((long *)&msgwhen);
	/*start outputting to qmail-queue
	  date is in 822 format
	 */
	fprintf(fdm,"Date: %u %s %u %02u:%02u:%02u -0000\nMessage-ID: <%lu.%u.autorespond@%s>\n"
		,dt->tm_mday,montab[dt->tm_mon],dt->tm_year+1900,dt->tm_hour,dt->tm_min,dt->tm_sec,msgwhen,getpid(),getenv("LOCAL") );

	mfp = fopen( msg, "rb" );
	
	while ( fgets( msg_buffer, sizeof(msg_buffer), mfp ) != NULL )
	{
		fprintf(fdm,"%s",msg_buffer);
	}

	fclose(mfp);

	fclose(fdm);


	/*send the envelopes*/

	fprintf(fde,"F%s",from);
	if(fwrite("",1,1,fde) != 1) {					/*write a null char*/
		fprintf(stderr, "AUTORESPOND: Reply failed to send from %s to %s: Failed to write envelope separator.\n", from, recipients[0]);
		fclose(fde);
		return -1;
	}
	for(i=0;i<num_recipients;i++) {
		fprintf(fde,"T%s",recipients[i]);
		if(fwrite("",1,1,fde) != 1) {					/*write a null char*/
			fprintf(stderr, "AUTORESPOND: Reply failed to send from %s to %s: Failed to write recipient separator.\n", from, recipients[0]);
			fclose(fde);
			return -1;
		}
	}
	if(fwrite("",1,1,fde) != 1) {					/*write a null char*/
		fprintf(stderr, "AUTORESPOND: Reply failed to send from %s to %s: Failed to write final separator.\n", from, recipients[0]);
		fclose(fde);
		return -1;
	}
	fclose(fde);

	/*wait for qmail-queue to close*/
	do {
		r = wait(&wstat);
	} while ((r != pid) && ((r != -1) || (errno == EINTR)));
	if(r != pid) {
		/*failed while waiting for qmail-queue*/
		fprintf(stderr, "AUTORESPOND: Reply failed to send from %s to %s: Failed while waiting for qmail-queue.\n", from, recipients[0]);
		return -1;
	}
	if(wstat & 127) {
		/*failed while waiting for qmail-queue*/
		fprintf(stderr, "AUTORESPOND: Reply failed to send from %s to %s: qmail-queue terminated by signal %d.\n", from, recipients[0], wstat & 127);
		return -1;
	}
	/*the exit code*/

	if((wstat >> 8)!=0) {
		/*non-zero exit status
		  failed while waiting for qmail-queue*/
		fprintf(stderr, "AUTORESPOND: Reply failed to send from %s to %s: qmail-queue exited with status %d.\n", from, recipients[0], wstat >> 8);
		return -1;
	}
	fprintf(stderr, "AUTORESPOND: Reply sent from %s to %s.\n", from, recipients[0]);
	return 0;
}

/****************************************************************
** reading header and break it down to it's tag and contet
   joins continued headers, stops on start of body  matthias@mhcsoftware.de
*/

void read_headers( FILE *fp )
{
	char h_buffer[HR_BUFFER_SIZE+1];
	char *ptr;
	int  len;
	headers *act_header = (headers *)NULL;


	while ( fgets( h_buffer, HR_BUFFER_SIZE, fp ) != NULL )
	{
		ptr = h_buffer;

		if ( *ptr == '\n' )
			break;

		switch( *ptr )
		{
		case ' ' :
		case '\t' : /* header continued */
			if (act_header == NULL) {
				/* Invalid header continuation without previous header */
				continue;
			} else {
				char* sanitized_continuation;
				
				sanitized_continuation = sanitize_header_content(ptr);
				if (!sanitized_continuation) {
					continue;
				}
				
				len = strlen( sanitized_continuation ) + strlen(act_header->content);
				if (len > 8192) { /* Prevent excessive header length */
					free(sanitized_continuation);
					continue;
				}
				
				act_header->content = safe_realloc( act_header->content, len + 1 );
				strncat( act_header->content, sanitized_continuation, len );
				act_header->content[len] = '\0';
				
				free(sanitized_continuation);
				
				/* Strip trailing newlines */
				len = strlen(act_header->content);
				while (len > 0 && (act_header->content[len-1] == '\n' || act_header->content[len-1] == '\r')) {
					act_header->content[len-1] = '\0';
					len--;
				}
			}
			break;
		default :
			if ( act_header != (headers *)NULL )
			{
				act_header->next = (headers*)safe_malloc( sizeof(headers) );
				act_header       = act_header->next;
			}
			else
				header = act_header = (headers*)safe_malloc( sizeof(headers) );

			act_header->next = (headers *)NULL;

			while( *ptr != ' ' && *ptr != '\t' && *ptr != ':' && *ptr != '\0' )
				ptr++;

			/* strip a possible : */
			len = ( *(ptr-1) == ':' ) ? ptr - h_buffer - 1 : ptr - h_buffer;

			/* Validate header tag */
			{
				char temp_tag[257];
				char* sanitized_content;
				
				if (len > 256) {
					/* Header tag too long, skip this header */
					act_header = act_header->next ? act_header->next : header;
					continue;
				}
				
				strncpy(temp_tag, h_buffer, len);
				temp_tag[len] = '\0';
				
				if (!validate_header_tag(temp_tag)) {
					/* Invalid header tag, skip this header */
					act_header = act_header->next ? act_header->next : header;
					continue;
				}

				/* skip whitspaces and colon */
				while( *ptr == ' ' || *ptr == '\t' || *ptr == ':' )
					ptr++;
		
				act_header->tag = (char*)safe_malloc( len + 1 );
				strncpy( act_header->tag, h_buffer, len );
				act_header->tag[len] = '\0';

				sanitized_content = sanitize_header_content(ptr);
				if (!sanitized_content) {
					/* Invalid header content, use empty string */
					act_header->content = (char*)safe_malloc(1);
					act_header->content[0] = '\0';
				} else {
					act_header->content = (char*)safe_malloc( strlen( sanitized_content ) + 1 );
					strcpy( act_header->content, sanitized_content );
					free(sanitized_content);
				}

				/* Strip trailing newlines */
				len = strlen(act_header->content);
				while (len > 0 && (act_header->content[len-1] == '\n' || act_header->content[len-1] == '\r')) {
					act_header->content[len-1] = '\0';
					len--;
				}
			}

			break;
		}
	}
}



/*********************************************************
** find string in string - ignore case **/

char *strcasestr2( char *_s1, char *_s2 )
{
	char *s1;
	char *s2;
	char *ptr;
	char *result = NULL;

	s1 = strdup(_s1);
	if (s1 == NULL) {
		return NULL;
	}
	
	s2 = strdup(_s2);
	if (s2 == NULL) {
		free(s1);
		return NULL;
	}

	for ( ptr = s1; *ptr != '\0'; ptr++ )
		*ptr = tolower( *ptr );

	for ( ptr = s2; *ptr != '\0'; ptr++ )
		*ptr = tolower( *ptr );

	ptr = strstr( s1, s2 );

	if ( ptr != (char *)NULL )
		result = _s1 + (ptr - s1);
	
	free(s1);
	free(s2);
	
	return result;
}




/*********************************************************
** look up header tag in chain and try to find search string 
** returns pointer to contetnt on success other wise NULL */

char *inspect_headers( char * tag, char *ss )
{
	headers *act_header;

	if (!tag) {
		return (char *)NULL;
	}

	act_header = header;

	while ( act_header != (headers *)NULL )
	{
		if ( strcasecmp( act_header->tag, tag ) == 0 )
		{
			if ( ss == (char *)NULL )
				return act_header->content;

			if ( strcasestr2( act_header->content, ss ) != (char *)NULL )
				return act_header->content;

			return (char *)NULL;
		}
		act_header = act_header->next;
	}
	return (char *)NULL;
}



/*********************************************************
** returns the content boundary string for this message */
char *get_content_boundary(void)
{
	char *s, *r;

	if ( (s = inspect_headers( "Content-Type", (char *)NULL )) == (char *)NULL) 
		return (char *)NULL;

	if ( (r = strcasestr2( s, "boundary=" )) == (char *)NULL)
		return (char *)NULL;
	
	*(r+strlen(r)-2) = '\0'; /* delete quote at the end */

	return r+10; /* point to first cahr after quote */
}



/*********************************************************
** concat headers to one string for output **/

char *return_header( char *tag )
{
	headers *act_header;
	int len;
	char *b;

	act_header = header;
	b     = (char *)safe_malloc( 20 );
	*b    = '\0';

	while ( act_header != (headers *)NULL )
	{
		if ( (tag != (char *)NULL && strcasecmp(tag,act_header->tag ) == 0) || tag == (char *)NULL )
		{
			/* Validate header content before concatenation */
			if (!act_header->tag || !act_header->content) {
				act_header = act_header->next;
				continue;
			}
			
			len = strlen( b ) + 
				  strlen( act_header->tag ) + 1 + 
				  strlen( act_header->content );

			/* Prevent excessive header concatenation */
			if (len > 16384) {
				break;
			}

			b = safe_realloc( b, len + 1);

			strcat( b, act_header->tag );
			strcat( b, ":" );
			strcat( b, act_header->content );

			b[len] = '\0';
		}
		act_header = act_header->next;
	}
	return( b );
}



/**********************************************************
** free header chain ***/

void free_headers(void)
{
	headers *act_header, *tmp_header;

	act_header = header;

	while ( act_header != (headers *)NULL )
	{
		free( act_header->tag );
		free( act_header->content );
		tmp_header = act_header;
		act_header = act_header->next;
		free( tmp_header );
	}
}


/**********************************************************
** print header chain for deguging ***/

void print_header_chain(void)
{
	headers *act_header;

	act_header = header;

	while ( act_header != (headers *)NULL )
	{
		printf( "%s: %s", act_header->tag, act_header->content );
		act_header = act_header->next;
	}
}



/**********************************************************
** regex_matches_header - Check if header matches sender filter list */

int regex_matches_header(const char *header_str) {
	regex_t regex;
	int ret;
	
	ret = regcomp(&regex, SENDER_FILTER_LIST, REG_EXTENDED | REG_ICASE);
	if (ret) return 0; /* return 0 on regex compilation error */
	
	ret = regexec(&regex, header_str, 0, NULL, 0);
	regfree(&regex);
	
	return (ret == 0); /* return 1 if match, 0 if no match */
}



/**********************************************************
** main */

int main(int argc, char ** argv)
{
char * sender;

char * message;
unsigned int time_message;
unsigned int timer;
unsigned int num;
char * message_filename;
char * dir;

char * ptr;
char * my_delivered_to;

DIR * dirp;
struct dirent * direntp;
unsigned int message_time;
char * address;
unsigned int count;
char filename[512];
FILE * f;
unsigned int message_handling = DEFAULT_MH;
char buffer[512];
char buffer2[512];
char *content_boundary;
char *rpath = DEFAULT_FROM;
char *TheUser;
char *TheDomain;

	if(argc > 7 || argc < 5) {
		fprintf(stderr, "\nautorespond: ");
		fprintf(stderr, "usage: time num message dir [ flag arsender ]\n\n");
		fprintf(stderr, "time - amount of time to consider a message (in seconds)\n");
		fprintf(stderr, "num - maximum number of messages to allow within time seconds\n");
		fprintf(stderr, "message - the filename of the message to send\n");
		fprintf(stderr, "dir - the directory to hold the log of messages\n\n");
		fprintf(stderr, "optional parameters:\n\n");
		fprintf(stderr, "flag - handling of original message:\n\n");
		fprintf(stderr, "0 - append nothing\n");
		fprintf(stderr, "1 - append quoted original message without attachments <default>\n\n");
		fprintf(stderr, "arsender - from address in generated message, or:\n\n");
		fprintf(stderr, "+ = blank from envelope !\n");
		fprintf(stderr, "$ = To: address will be used\n\n");
		_exit(111);
	}

	TheUser= getenv("EXT");
	TheDomain= getenv("HOST");
	
	/* Validate environment variables */
	if (!TheUser) TheUser = "unknown";
	if (!TheDomain) TheDomain = "localhost";
 
	setvbuf(stderr, NULL, _IONBF, 0);
	
	/* Initialize random seed for secure temporary file creation */
	srandom((unsigned int)time(NULL) ^ getpid());

	if(argc > 7 || argc < 5) {
		fprintf(stderr, "AUTORESPOND: Invalid arguments. (%d)\n",argc);
		_exit(111);
	}

	time_message     = strtoul(argv[1],NULL,10);
	num              = strtoul(argv[2],NULL,10);
	message_filename = argv[3];
	dir              = argv[4];

	if ( argc > 5 )
		message_handling = strtoul(argv[5],NULL,10);
	if ( argc > 6 )
		rpath = argv[6];

	/* Validate directory path to prevent directory traversal */
	if (!validate_directory_path(dir)) {
		fprintf(stderr, "AUTORESPOND: Invalid directory path.\n");
		_exit(111);
	}
	
	/* Validate message handling parameter */
	if (message_handling > 1) {
		fprintf(stderr, "AUTORESPOND: Invalid message handling flag.\n");
		_exit(111);
	}

	if ( *rpath == '+' )
		rpath = "";
	if ( *rpath == '$' )
	{
		snprintf(buffer2, sizeof(buffer2), "%s@%s", TheUser, TheDomain);
		rpath = buffer2;
	}

	timer = time(NULL);

	/*prepare the "delivered-to" string*/
	my_delivered_to = "Delivered-To: Autoresponder\n";

	read_headers( stdin );


	message = read_file(message_filename);
	if(message==NULL) {
		fprintf(stderr, "AUTORESPOND: Failed to open message file.\n");
		_exit(111);
	}

	/*don't autorespond in certain situations*/
	sender = getenv("SENDER");
	if(sender==NULL)
		sender = "";

	/*don't autorespond to a mailer-daemon*/
	if( sender[0]==0 || strncasecmp(sender,"mailer-daemon",13)==0 || strchr(sender,'@')==NULL || strcmp(sender,"#@[]")==0 ) {
		/*exit with success and continue parsing .qmail file*/
		fprintf(stderr,"AUTORESPOND:  Stopping on mail from [%.*s].\n", 100, sender);
		_exit(0);
	}
	
	/* Validate sender email address */
	if (!validate_email_address(sender)) {
		fprintf(stderr, "AUTORESPOND: Invalid sender email address format.\n");
		_exit(0);
	}


	if ( inspect_headers("mailing-list", (char *)NULL ) != (char *)NULL )
	{
		fprintf(stderr,"AUTORESPOND: This looks like it's from a mailing list, I will ignore it.\n");
		_exit(0);			/*report success and exit*/
	}
	if ( inspect_headers("Delivered-To", "Autoresponder" ) != (char *)NULL )
	{
		/*got one of my own messages...*/
		fprintf(stderr,"AUTORESPOND: This message is looping...it has my Delivered-To header.\n");
		_exit(100);			/*hard error*/
	}
	if ( inspect_headers("precedence", "junk" ) != (char *)NULL ||
		 inspect_headers("precedence", "bulk" ) != (char *)NULL ||
		 inspect_headers("precedence", "list" ) != (char *)NULL )
	{
		fprintf(stderr,"AUTORESPOND: Junk mail received.\n");
		_exit(0); /* don't reply to bulk, junk, or list mail */
	}
	
	/* Check for List-Id header */
	if ( inspect_headers("list-id", (char *)NULL ) != (char *)NULL )
	{
		fprintf(stderr,"AUTORESPOND: Message has List-Id header, ignoring.\n");
		_exit(0);
	}
	
	/* Check for List-Unsubscribe header */
	if ( inspect_headers("list-unsubscribe", (char *)NULL ) != (char *)NULL )
	{
		fprintf(stderr,"AUTORESPOND: Message has List-Unsubscribe header, ignoring.\n");
		_exit(0);
	}
	
	/* Check for X-Report-Abuse-To header. This is most commonly associated with
	transactional messages, although it is used by a very small number of boutique email
	hosting providers. There will be false positives. */
	if ( inspect_headers("x-report-abuse-to", (char *)NULL ) != (char *)NULL )
	{
		fprintf(stderr,"AUTORESPOND: Message has X-Report-Abuse-To header, ignoring.\n");
		_exit(0);
	}
	
	/* Check for X-Patreon-UUID header */
	if ( inspect_headers("x-patreon-uuid", (char *)NULL ) != (char *)NULL )
	{
		fprintf(stderr,"AUTORESPOND: Message has X-Patreon-UUID header, ignoring.\n");
		_exit(0);
	}
	
	/* Check for X-Mailgun-Tag header */
	if ( inspect_headers("x-mailgun-tag", (char *)NULL ) != (char *)NULL )
	{
		fprintf(stderr,"AUTORESPOND: Message has X-Mailgun-Tag header, ignoring.\n");
		_exit(0);
	}
	
	/* Check X-Spam-Level for asterisks */
	ptr = inspect_headers("x-spam-level", (char *)NULL );
	if ( ptr != NULL && strchr( ptr, '*' ) != NULL )
	{
		fprintf(stderr,"AUTORESPOND: X-Spam-Level header contains asterisk, ignoring: %s.\n", ptr);
		_exit(0);
	}
	
	/* Check for CLI-based mail agents */
	ptr = inspect_headers("user-agent", (char *)NULL );
	if ( ptr != NULL && (strstr( ptr, "mailx" ) != NULL || strstr( ptr, "s-nail" ) != NULL) )
	{
		fprintf(stderr,"AUTORESPOND: User-Agent header contains CLI-based mail agent, ignoring: %s.\n", ptr);
		_exit(0);
	}
	
	/* Check Sender header against filter list */
	ptr = inspect_headers("sender", (char *)NULL );
	if ( ptr != NULL && regex_matches_header( ptr ) )
	{
		fprintf(stderr,"AUTORESPOND: Sender header matches filter list, ignoring: %s.\n", ptr);
		_exit(0);
	}
	
	/* Check From header against filter list */
	ptr = inspect_headers("from", (char *)NULL );
	if ( ptr != NULL && regex_matches_header( ptr ) )
	{
		fprintf(stderr,"AUTORESPOND: From header matches filter list, ignoring: %s.\n", ptr);
		_exit(0);
	}
	
	/* Check Reply-To header against filter list */
	ptr = inspect_headers("reply-to", (char *)NULL );
	if ( ptr != NULL && regex_matches_header( ptr ) )
	{
		fprintf(stderr,"AUTORESPOND: Reply-To header matches filter list, ignoring: %s.\n", ptr);
		_exit(0);
	}
	
	/* Check Return-Path header against filter list */
	ptr = inspect_headers("return-path", (char *)NULL );
	if ( ptr != NULL && regex_matches_header( ptr ) )
	{
		fprintf(stderr,"AUTORESPOND: Return-Path header matches filter list, ignoring: %s.\n", ptr);
		_exit(0);
	}

	/*check the logs*/
	if(chdir(dir) == -1) {
		fprintf(stderr,"AUTORESPOND: Failed to change into directory.\n");
		_exit(111);
	}
	
	/* Verify we're in the expected directory and add entry */
	{
		char cwd_buffer[PATH_MAX];
		int log_fd;
		
		if (!getcwd(cwd_buffer, sizeof(cwd_buffer))) {
			fprintf(stderr,"AUTORESPOND: Unable to verify current directory.\n");
			_exit(111);
		}

		/*add entry*/
		log_fd = create_secure_temp_file(filename, sizeof(filename), "A");
		if(log_fd == -1) {
			fprintf(stderr,"AUTORESPOND: Unable to create secure log file for [%.*s].", 100, sender);
			_exit(111);
		}
		f = fdopen(log_fd, "wb");
		if(f==NULL) {
			fprintf(stderr,"AUTORESPOND: Unable to open log file stream for [%.*s].", 100, sender);
			close(log_fd);
			unlink(filename);
			_exit(111);
		}
		if(fwrite(sender,1,strlen(sender),f)!=strlen(sender)) {
			fprintf(stderr,"AUTORESPOND: Unable to create file for [%.*s].", 100, sender);
			fclose(f);
			unlink(filename);
			_exit(111);
		}
		fclose(f);
	}

	/*check if there are too many responses in the logs*/
	dirp = opendir(".");
	count = 0;
	while((direntp = readdir(dirp)) != NULL) {
		if(direntp->d_name[0] != 'A')
			continue;
		ptr = strchr(direntp->d_name,'.');
		if(ptr==NULL)
			continue;
		message_time = strtoul(ptr+1,NULL,10);
		if(message_time < timer-time_message) {
			/*too old..ignore errors on unlink*/
			unlink(direntp->d_name);
		} else {
			address = read_file(direntp->d_name);
			if(address==NULL) {
				/*ignore this?*/
				continue;
			}
			if(strcasecmp(address,sender)==0) {
				count++;
			}
			free(address);
		}
	}
	
	if(count>num) {
		fprintf(stderr,"AUTORESPOND: too many received from [%.*s]\n", 100, sender);
		_exit(0); /* don't reply to this message, but allow it to be delivered */
	}

	/* Create temporary file for response */
	{
		int temp_fd;
		
		temp_fd = create_secure_temp_file(filename, sizeof(filename), "tmp");
		if(temp_fd == -1) {
			fprintf(stderr,"AUTORESPOND: Unable to create secure temporary file.\n");
			_exit(111);
		}
		f = fdopen(temp_fd, "wb");
		if(f==NULL) {
			fprintf(stderr,"AUTORESPOND: Unable to open temporary file stream.\n");
			close(temp_fd);
			unlink(filename);
			_exit(111);
		}

		fprintf( f, "%sTo: %s\nX-Original-From: %s\nX-Original-Subject: Re:%s\n%s\n", 
			my_delivered_to, sender, rpath, inspect_headers( "Subject", (char *) NULL ), message );

		if ( message_handling == 1 ) {
			fprintf( f, "%s\n\n", "-------- Original Message --------" );
			if ( (content_boundary = get_content_boundary()) == (char *)NULL )
			{
				while ( fgets( buffer, sizeof(buffer), stdin ) != NULL )
				{
					fputs( "> ", f );
					fputs( buffer, f );
				}
			} else
			{
				int content_found = 0;
				while ( fgets( buffer, sizeof(buffer), stdin ) != NULL )
				{
					if ( content_found == 1 ) 
					{
						if ( strstr( buffer, content_boundary ) != (char *)NULL )
							break;
						fputs( "> ", f );
						fputs( buffer, f );
					}
					if ( strstr( buffer, content_boundary ) != (char *)NULL )
					{
						if ( content_found == 1 )
							break;
						else
						{
							free_headers();
							read_headers( stdin );
							if ( inspect_headers("Content-Type", "text/plain" ) != (char *)NULL )
								content_found = 1;
							continue;
						}
					}
				}
			}
		}

		fprintf( f, "\n\n" );

		fclose( f );

		/*send the autoresponse...ignore errors?*/
		send_message(filename,rpath,&sender,1); 

		unlink( filename );
	}

	_exit(0);
	return 0;					/*compiler warning squelch*/
}
