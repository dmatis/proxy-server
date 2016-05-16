#include <stdio.h>

int banned_sites_couter;
char** banned_sites;

char fqdn[256];
char subdir[256];
int client_port;
char* strptr;
char server_reply[4096], reply_cpy[4096];
char request_type[4];
int hasPort = 0;

void parse_fqdn_subdir_clientport()
{
	strcpy(reply_cpy, server_reply);
    strptr = strtok(reply_cpy, " ");
    strcpy(request_type, strptr);
    strptr = strtok(NULL, "//");
    strptr = strtok(NULL, "/");        
    //Handles if port included in request
    if (strstr(strptr,":") != 0){
        hasPort = 1;
    } else {
        //Default Port is 80
        client_port = 80;
    }
    //Handles the case when dealing with root website only (no subdir)
    //(ie: http://www.example.com)
    if(strstr(strptr,"HTTP") != 0){
        //We have root website only
        strcpy(reply_cpy, server_reply);
        strptr = strtok(reply_cpy, " ");
        strptr = strtok(NULL, "//");
        if(hasPort){
            strptr = strtok(NULL, ":");
            strcpy(fqdn, strptr+1);
            strptr = strtok(NULL, " ");
            client_port = atoi(strptr);
        } else {
            strptr = strtok(NULL, " ");
            strcpy(fqdn, strptr+1);
            strptr = strtok(NULL, " ");
        }
        strcpy(subdir, "");
        } else {
        strcpy(fqdn, strptr);
        if(hasPort){
            strptr = strtok(NULL, ":");
            strcpy(subdir, strptr);
            strptr = strtok(NULL, " ");
            client_port = atoi(strptr);
        } else {
            strptr = strtok(NULL, " ");
            strcpy(subdir, strptr);
        }
    }	
}

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

	banned_sites_couter = 0;
    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            banned_sites_couter++;
			assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

//Checks for blacklisted sites
int is_banned(char* fqdn){        
	int found_site = 0;
	int l = 0;
	char* holder;
	for(;l<banned_sites_couter;l++){
		holder =  (char *)malloc(sizeof(char) * 100);
		stpcpy(holder, banned_sites[l]);
		if(l+1<banned_sites_couter)
		{
			holder[strlen(holder)-1] = 0;
		}
		if(strstr(fqdn, holder) != 0)
		{
			puts("403 blacklisted site");
			found_site = 1;
			return found_site;
		}
	}
	return found_site;
}
