/* Author : KK Nidhin
 Date : 31/08/2023
*/

// Include libraries
#include <syslog.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main(int argc, char** argv)
{
	// Check if the parameters are correct
	if(argc != 3)
	{
		// Write to syslog as LOG_ERR
		syslog(LOG_ERR,"This program requires 2 arguments\n");
		return 1;
	}

	// Create a file handler with write permission
	FILE *f = fopen(argv[1], "w+");
	// Write to the syslog as LOG_USER
	syslog(LOG_USER,"Writing %s to %s\n", argv[2], argv[1]);
	// Write the content to the file
	fwrite(argv[2], strlen(argv[2]), 1, f);

	return 0;
}
