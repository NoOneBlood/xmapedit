/*
 * Playing-field leveller for Build
 * by Jonathon Fowler
 *
 * A note about this:
 * 1. There is some kind of problem somewhere in the functions below because
 *    compiling with __compat_h_macrodef__ disabled makes stupid things happen.
 * 2. The functions below, aside from the ones which aren't trivial, were never
 *    really intended to be used for anything except tracking anr removing ties
 *    to the standard C library from games. Using the Bxx versions of functions
 *    means we can redefine those names to link up with different runtime library
 *    names.
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#if SUBSYS == 400 || SUBSYS == 401
	#define WINVER 0x0400
	#define _WIN32_IE 0x0400
#elif SUBSYS == 501
	#define WINVER 0x0501
	#define _WIN32_IE 0x0501
	#define _WIN32_WINNT 0x0501
#else
	#define WINVER 0x0600
	#define _WIN32_IE 0x0600
	#define _WIN32_WINNT 0x0600
#endif
#include <windows.h>
#include <shlobj.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef __APPLE__
# include "osxbits.h"
#endif

#if defined(__WATCOMC__)
# include <direct.h>
#elif defined(_MSC_VER)
#else
# include <dirent.h>
# ifdef _WIN32
#  include <direct.h>	// for _getdcwd
# endif
#endif

#if defined(__linux) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#  include <libgen.h> // for dirname()
#endif

#if defined(__FreeBSD__)
#  include <sys/sysctl.h> // for sysctl() to get path to executable
#endif

#include "compat.h"


#ifndef __compat_h_macrodef__

int Brand(void)
{
	return rand();
}

void *Bmalloc(bsize_t size)
{
	return malloc(size);
}

void Bfree(void *ptr)
{
	free(ptr);
}

int Bopen(const char *pathname, int flags, unsigned mode)
{
	int n=0,o=0;
	
	if (flags&BO_BINARY) n|=O_BINARY; else n|=O_TEXT;
	if ((flags&BO_RDWR)==BO_RDWR) n|=O_RDWR;
	else if ((flags&BO_RDWR)==BO_WRONLY) n|=O_WRONLY;
	else if ((flags&BO_RDWR)==BO_RDONLY) n|=O_RDONLY;
	if (flags&BO_APPEND) n|=O_APPEND;
	if (flags&BO_CREAT) n|=O_CREAT;
	if (flags&BO_TRUNC) n|=O_TRUNC;
	if (mode&BS_IREAD) o|=S_IREAD;
	if (mode&BS_IWRITE) o|=S_IWRITE;
	if (mode&BS_IEXEC) o|=S_IEXEC;
	
	return open(pathname,n,o);
}

int Bclose(int fd)
{
	return close(fd);
}

bssize_t Bwrite(int fd, const void *buf, bsize_t count)
{
	return write(fd,buf,count);
}

bssize_t Bread(int fd, void *buf, bsize_t count)
{
	return read(fd,buf,count);
}

boff_t Blseek(int fildes, boff_t offset, int whence)
{
	switch (whence) {
		case BSEEK_SET: whence=SEEK_SET; break;
		case BSEEK_CUR: whence=SEEK_CUR; break;
		case BSEEK_END: whence=SEEK_END; break;
	}
	return lseek(fildes,offset,whence);
}

BFILE *Bfopen(const char *path, const char *mode)
{
	return (BFILE*)fopen(path,mode);
}

int Bfclose(BFILE *stream)
{
	return fclose((FILE*)stream);
}

void Brewind(BFILE *stream)
{
	rewind((FILE*)stream);
}

int Bfgetc(BFILE *stream)
{
	return fgetc((FILE*)stream);
}

char *Bfgets(char *s, int size, BFILE *stream)
{
	return fgets(s,size,(FILE*)stream);
}

int Bfputc(int c, BFILE *stream)
{
	return fputc(c,(FILE*)stream);
}

int Bfputs(const char *s, BFILE *stream)
{
	return fputs(s,(FILE*)stream);
}

bsize_t Bfread(void *ptr, bsize_t size, bsize_t nmemb, BFILE *stream)
{
	return fread(ptr,size,nmemb,(FILE*)stream);
}

bsize_t Bfwrite(const void *ptr, bsize_t size, bsize_t nmemb, BFILE *stream)
{
	return fwrite(ptr,size,nmemb,(FILE*)stream);
}


char *Bstrdup(const char *s)
{
	return strdup(s);
}

char *Bstrcpy(char *dest, const char *src)
{
	return strcpy(dest,src);
}

char *Bstrncpy(char *dest, const char *src, bsize_t n)
{
	return strncpy(dest,src,n);
}

int Bstrcmp(const char *s1, const char *s2)
{
	return strcmp(s1,s2);
}

int Bstrncmp(const char *s1, const char *s2, bsize_t n)
{
	return strncmp(s1,s2,n);
}

int Bstrcasecmp(const char *s1, const char *s2)
{
#ifdef _MSC_VER
	return stricmp(s1,s2);
#else
	return strcasecmp(s1,s2);
#endif
}

int Bstrncasecmp(const char *s1, const char *s2, bsize_t n)
{
#ifdef _MSC_VER
	return strnicmp(s1,s2,n);
#else
	return strncasecmp(s1,s2,n);
#endif
}

char *Bstrcat(char *dest, const char *src)
{
	return strcat(dest,src);
}

char *Bstrncat(char *dest, const char *src, bsize_t n)
{
	return strncat(dest,src,n);
}

bsize_t Bstrlen(const char *s)
{
	return strlen(s);
}

char *Bstrchr(const char *s, int c)
{
	return strchr(s,c);
}

char *Bstrrchr(const char *s, int c)
{
	return strrchr(s,c);
}

int Batoi(const char *nptr)
{
	return atoi(nptr);
}

long Batol(const char *nptr)
{
	return atol(nptr);
}

long int Bstrtol(const char *nptr, char **endptr, int base)
{
	return strtol(nptr,endptr,base);
}

unsigned long int Bstrtoul(const char *nptr, char **endptr, int base)
{
	return strtoul(nptr,endptr,base);
}

void *Bmemcpy(void *dest, const void *src, bsize_t n)
{
	return memcpy(dest,src,n);
}

void *Bmemmove(void *dest, const void *src, bsize_t n)
{
	return memmove(dest,src,n);
}

void *Bmemchr(const void *s, int c, bsize_t n)
{
	return memchr(s,c,n);
}

void *Bmemset(void *s, int c, bsize_t n)
{
	return memset(s,c,n);
}

int Bprintf(const char *format, ...)
{
	va_list ap;
	int r;

	va_start(ap,format);
#ifdef _MSC_VER
	r = _vprintf(format,ap);
#else
	r = vprintf(format,ap);
#endif
	va_end(ap);
	return r;
}

int Bsprintf(char *str, const char *format, ...)
{
	va_list ap;
	int r;

	va_start(ap,format);
#ifdef _MSC_VER
	r = _vsprintf(str,format,ap);
#else
	r = vsprintf(str,format,ap);
#endif
	va_end(ap);
	return r;
}

int Bsnprintf(char *str, bsize_t size, const char *format, ...)
{
	va_list ap;
	int r;

	va_start(ap,format);
#ifdef _MSC_VER
	r = _vsnprintf(str,size,format,ap);
#else
	r = vsnprintf(str,size,format,ap);
#endif
	va_end(ap);
	return r;
}

int Bvsnprintf(char *str, bsize_t size, const char *format, va_list ap)
{
#ifdef _MSC_VER
	return _vsnprintf(str,size,format,ap);
#else
	return vsnprintf(str,size,format,ap);
#endif
}

char *Bgetenv(const char *name)
{
	return getenv(name);
}

char *Bgetcwd(char *buf, bsize_t size)
{
	return getcwd(buf,size);
}

#endif	// __compat_h_macrodef__


//
// Stuff which must be a function
//

/**
 * Get the location of the user's home/profile data directory.
 * The caller must free the string when done with it.
 * @return NULL if it could not be determined
 */
char *Bgethomedir(void)
{
    char *dir = NULL;

#ifdef _WIN32
	#ifdef _WIN32_WINNT
		TCHAR appdata[MAX_PATH];
		if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appdata)))
			dir = strdup(appdata);
	#else
		dir = Bgetappdir();
	#endif
#elif defined __APPLE__
    dir = osx_gethomedir();
#else
	char *e = getenv("HOME");
	if (e) {
        dir = strdup(e);
    }
#endif
	return dir;
}

/**
 * Get the location of the application directory.
 * On OSX this is the .app bundle resource directory.
 * On Windows this is the directory the executable was launched from.
 * On Linux/BSD it's the executable's directory
 * The caller must free the string when done with it.
 * @return NULL if it could not be determined
 */
char *Bgetappdir(void)
{
    char *dir = NULL;
    
#ifdef _WIN32
	TCHAR appdir[MAX_PATH];
    
	if (GetModuleFileName(NULL, appdir, MAX_PATH) > 0) {
		// trim off the filename
		char *slash = strrchr(appdir, '\\');
		if (slash) slash[0] = 0;
		dir = strdup(appdir);
    }

#elif defined __APPLE__
    dir = osx_getappdir();
    
#elif defined(__linux) || defined(__NetBSD__) || defined(__OpenBSD__)
    char buf[PATH_MAX] = {0};
    char buf2[PATH_MAX] = {0};
#  ifdef __linux
    snprintf(buf, sizeof(buf), "/proc/%d/exe", getpid());
#  else // the BSDs.. except for FreeBSD which has a sysctl
    snprintf(buf, sizeof(buf), "/proc/%d/file", getpid());
#  endif
    int len = readlink(buf, buf2, sizeof(buf2));
    if (len != -1) {
        // remove executable name with dirname(3)
        // on Linux, dirname() will modify buf2 (cutting off executable name) and return it
        // on FreeBSD it seems to use some internal buffer instead.. anyway, just strdup()
        dir = strdup(dirname(buf2));
    }
#elif defined(__FreeBSD__)
    // the sysctl should also work when /proc/ is not mounted (which seems to
    // be common on FreeBSD), so use it..
    char buf[PATH_MAX] = {0};
    int name[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
    size_t len = sizeof(buf)-1;
    int ret = sysctl(name, sizeof(name)/sizeof(name[0]), buf, &len, NULL, 0);
    if(ret == 0 && buf[0] != '\0') {
        // again, remove executable name with dirname()
        // on FreeBSD dirname() seems to use some internal buffer
        dir = strdup(dirname(buf));
    }
#endif
    
    return dir;
}

/**
 * Get the location for global or user-local support files.
 * The caller must free the string when done with it.
 * @return NULL if it could not be determined
 */
char *Bgetsupportdir(int global)
{
    char *dir = NULL;
    
#ifdef __APPLE__
    dir = osx_getsupportdir(global);

#else
    if (!global) {
        dir = Bgethomedir();
    }
#endif
    
	return dir;
}

int Bcorrectfilename(char *filename, int removefn)
{
	char *fn;
	const int MAXTOKARR = 64;
	char *tokarr[64], *first, *next, *token;
	int i, ntok = 0, leadslash = 0, trailslash = 0;
	
	fn = strdup(filename);
	if (!fn) return -1;
	
	// find the end of the string
	for (first=fn; *first; first++) {
#ifdef _WIN32
		// translating backslashes to forwardslashes on the way
		if (*first == '\\') *first = '/';
#endif
	}
	leadslash = (*fn == '/');
	trailslash = (first>fn && first[-1] == '/');
	
	// carve up the string into pieces by directory, and interpret
	// the . and .. components
	first = fn;
	do {
		token = Bstrtoken(first, "/", &next, 1);
		first = NULL;
		if (!token) break;
		else if (token[0] == 0) continue;
		else if (token[0] == '.' && token[1] == 0) continue;
		else if (token[0] == '.' && token[1] == '.' && token[2] == 0) ntok = max(0,ntok-1);
		else tokarr[ntok++] = token;
	} while (ntok < MAXTOKARR);
	
	if (!trailslash && removefn) { ntok = max(0,ntok-1); trailslash = 1; }
	if (ntok == 0 && trailslash && leadslash) trailslash = 0;
	
	// rebuild the filename
	first = filename;
	if (leadslash) *(first++) = '/';
	for (i=0; i<ntok; i++) {
		if (i>0) *(first++) = '/';
		for (token=tokarr[i]; *token; token++)
			*(first++) = *token;
	}
	if (trailslash) *(first++) = '/';
	*(first++) = 0;
	
	free(fn);

	return 0;
}

int Bcanonicalisefilename(char *filename, int removefn)
{
	char cwd[BMAX_PATH], fn[BMAX_PATH], *p;
	char *fnp = filename;
#ifdef _WIN32
	int drv = 0;
#endif
	
#ifdef _WIN32
	{
		if (filename[0] && filename[1] == ':') {
			// filename is prefixed with a drive
			drv = toupper(filename[0])-'A' + 1;
			fnp += 2;
		}
		if (!_getdcwd(drv, cwd, sizeof(cwd))) return -1;
		for (p=cwd; *p; p++) if (*p == '\\') *p = '/';
	}
#else
	if (!getcwd(cwd,sizeof(cwd))) return -1;
#endif
	p = strrchr(cwd,'/'); if (!p || p[1]) strcat(cwd, "/");
	
	strcpy(fn, fnp);
#ifdef _WIN32
	for (p=fn; *p; p++) if (*p == '\\') *p = '/';
#endif
	
	if (fn[0] != '/') {
		// we are dealing with a path relative to the current directory
		strcpy(filename, cwd);
		strcat(filename, fn);
	} else {
#ifdef _WIN32
		filename[0] = cwd[0];
		filename[1] = ':';
		filename[2] = 0;
#else
		filename[0] = 0;
#endif
		strcat(filename, fn);
	}
	fnp = filename;
#ifdef _WIN32
	fnp += 2;	// skip the drive
#endif
	
	return Bcorrectfilename(fnp,removefn);
}

char *Bgetsystemdrives(void)
{
#ifdef _WIN32
	char *str, *p;
	DWORD drv, mask;
	int number=0;
	
	drv = GetLogicalDrives();
	if (drv == 0) return NULL;

	for (mask=1; mask<0x8000000l; mask<<=1) {
		if ((drv&mask) == 0) continue;
		number++;
	}

	str = p = (char *)malloc(1 + (3*number));
	if (!str) return NULL;

	number = 0;
	for (mask=1; mask<0x8000000l; mask<<=1, number++) {
		if ((drv&mask) == 0) continue;
		*(p++) = 'A' + number;
		*(p++) = ':';
		*(p++) = 0;
	}
	*(p++) = 0;

	return str;
#else
	// Perhaps have Unix OS's put /, /home/user, and /mnt/* in the "drives" list?
	return NULL;
#endif
}


boff_t Bfilelength(int fd)
{
	struct stat st;
	if (fstat(fd, &st) < 0) return -1;
	return(boff_t)(st.st_size);
}


typedef struct {
#ifdef _MSC_VER
	HANDLE hfind;
	WIN32_FIND_DATA fid;
#else
	DIR *dir;
	int rootlen;
	char *work;
	int worklen;
#endif
	struct Bdirent info;
	int status;
} BDIR_real;

BDIR* Bopendir(const char *name)
{
	BDIR_real *dirr;

#ifdef _MSC_VER
	char *tname, *tcurs;
#endif

	dirr = (BDIR_real*)malloc(sizeof(BDIR_real));
	if (!dirr) {
		return NULL;
	}
	memset(dirr, 0, sizeof(BDIR_real));

#ifdef _MSC_VER
	tname = (char*)malloc(strlen(name) + 4 + 1);
	if (!tname) {
		free(dirr);
		return NULL;
	}

	strcpy(tname, name);
	for (tcurs = tname; *tcurs; tcurs++) ;	// Find the end of the string.
	tcurs--;	// Step back off the null char.
	while (*tcurs == ' ' && tcurs>tname) tcurs--;	// Remove any trailing whitespace.
	if (*tcurs != '/' && *tcurs != '\\') *(++tcurs) = '/';
	*(++tcurs) = '*';
	*(++tcurs) = '.';
	*(++tcurs) = '*';
	*(++tcurs) = 0;
	
	dirr->hfind = FindFirstFile(tname, &dirr->fid);
	free(tname);
	if (dirr->hfind == INVALID_HANDLE_VALUE) {
		free(dirr);
		return NULL;
	}
#else
	dirr->dir = opendir(name);
	if (dirr->dir == NULL) {
		free(dirr);
		return NULL;
	}

	// Preallocate the work buffer.
	dirr->rootlen = strlen(name);
	dirr->worklen = dirr->rootlen + 1 + 64 + 1;
	dirr->work = (char *)malloc(dirr->worklen);
	if (!dirr->work) {
		closedir(dirr->dir);
		free(dirr);
		return NULL;
	}

	strcpy(dirr->work, name);
	strcat(dirr->work, "/");
#endif

	dirr->status = 0;
	
	return (BDIR*)dirr;
}

struct Bdirent*	Breaddir(BDIR *dir)
{
	BDIR_real *dirr = (BDIR_real*)dir;

#ifdef _MSC_VER
	LARGE_INTEGER tmp;

	if (dirr->status > 0) {
		if (FindNextFile(dirr->hfind, &dirr->fid) == 0) {
			dirr->status = -1;
			return NULL;
		}
	}
	dirr->info.namlen = strlen(dirr->fid.cFileName);
	dirr->info.name = (char *)dirr->fid.cFileName;
	
	dirr->info.mode = 0;
	if (dirr->fid.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) dirr->info.mode |= BS_IFDIR;
	else dirr->info.mode |= BS_IFREG;
	if (dirr->fid.dwFileAttributes & FILE_ATTRIBUTE_READONLY) dirr->info.mode |= S_IREAD;
	else dirr->info.mode |= S_IREAD|S_IWRITE|S_IEXEC;

	tmp.HighPart = dirr->fid.nFileSizeHigh;
	tmp.LowPart = dirr->fid.nFileSizeLow;
	dirr->info.size = (boff_t)tmp.QuadPart;

	tmp.HighPart = dirr->fid.ftLastWriteTime.dwHighDateTime;
	tmp.LowPart = dirr->fid.ftLastWriteTime.dwLowDateTime;
	tmp.QuadPart -= INT64_C(116444736000000000);
	dirr->info.mtime = (btime_t)(tmp.QuadPart / 10000000);
	
	dirr->status++;

#else
    struct dirent *de;
    struct stat st;
    int fnlen;

    de = readdir(dirr->dir);
    if (de == NULL) {
		dirr->status = -1;
		return NULL;
	} else {
		dirr->status++;
	}

	dirr->info.namlen = strlen(de->d_name);
	dirr->info.name   = de->d_name;
	dirr->info.mode = 0;
	dirr->info.size = 0;
	dirr->info.mtime = 0;

	fnlen = dirr->rootlen + 1 + dirr->info.namlen + 1;
	if (dirr->worklen < fnlen) {
		char *newwork = (char *)realloc(dirr->work, fnlen);
		if (!newwork) {
			dirr->status = -1;
			return NULL;
		}

		dirr->work = newwork;
		dirr->worklen = fnlen;
	}

	strcpy(&dirr->work[dirr->rootlen + 1], dirr->info.name);
	if (!stat(dirr->work, &st)) {
        dirr->info.mode = st.st_mode;
        dirr->info.size = st.st_size;
        dirr->info.mtime = st.st_mtime;
	}
#endif

	return &dirr->info;
}

int Bclosedir(BDIR *dir)
{
	BDIR_real *dirr = (BDIR_real*)dir;
	
#ifdef _MSC_VER
	FindClose(dirr->hfind);
#else
	free(dirr->work);
	closedir(dirr->dir);
#endif
	free(dirr);

	return 0;
}


char *Bstrtoken(char *s, char *delim, char **ptrptr, int chop)
{
	char *p, *start;

	if (!ptrptr) return NULL;
	
	if (s) p = s;
	else p = *ptrptr;

	if (!p) return NULL;

	while (*p != 0 && strchr(delim, *p)) p++;
	if (*p == 0) {
		*ptrptr = NULL;
		return NULL;
	}
	start = p;
	while (*p != 0 && !strchr(delim, *p)) p++;
	if (*p == 0) *ptrptr = NULL;
	else {
		if (chop) *(p++) = 0;
		*ptrptr = p;
	}

	return start;
}


	//Brute-force case-insensitive, slash-insensitive, * and ? wildcard matcher
	//Given: string i and string j. string j can have wildcards
	//Returns: 1:matches, 0:doesn't match
int Bwildmatch (const char *i, const char *j)
{
	const char *k;
	char c0, c1;

	if (!*j) return(1);
	do
	{
		if (*j == '*')
		{
			for(k=i,j++;*k;k++) if (Bwildmatch(k,j)) return(1);
			continue;
		}
		if (!*i) return(0);
		if (*j == '?') { i++; j++; continue; }
		c0 = *i; if ((c0 >= 'a') && (c0 <= 'z')) c0 -= 32;
		c1 = *j; if ((c1 >= 'a') && (c1 <= 'z')) c1 -= 32;
#ifdef _WIN32
		if (c0 == '/') c0 = '\\';
		if (c1 == '/') c1 = '\\';
#endif
		if (c0 != c1) return(0);
		i++; j++;
	} while (*j);
	return(!*i);
}

#if !defined(_WIN32)
char *Bstrlwr(char *s)
{
	char *t = s;
	if (!s) return s;
	while (*t) { *t = Btolower(*t); t++; }
	return s;
}

char *Bstrupr(char *s)
{
	char *t = s;
	if (!s) return s;
	while (*t) { *t = Btoupper(*t); t++; }
	return s;
}
#endif


//
// getsysmemsize() -- gets the amount of system memory in the machine
//
size_t Bgetsysmemsize(void)
{
#ifdef _WIN32
	size_t siz = 0x7fffffff;
	
        MEMORYSTATUSEX memst;
        memst.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memst)) {
            siz = (size_t)min(INT64_C(0x7fffffff), memst.ullTotalPhys);
        }
	
	return siz;
#elif (defined(_SC_PAGE_SIZE) || defined(_SC_PAGESIZE)) && defined(_SC_PHYS_PAGES)
	size_t siz = 0x7fffffff;
	long scpagesiz, scphyspages;

#ifdef _SC_PAGE_SIZE
	scpagesiz = sysconf(_SC_PAGE_SIZE);
#else
	scpagesiz = sysconf(_SC_PAGESIZE);
#endif
	scphyspages = sysconf(_SC_PHYS_PAGES);
	if (scpagesiz >= 0 && scphyspages >= 0)
		siz = (size_t)min(INT64_C(0x7fffffff), (int64_t)scpagesiz * (int64_t)scphyspages);

	//buildprintf("Bgetsysmemsize(): %d pages of %d bytes, %d bytes of system memory\n",
	//		scphyspages, scpagesiz, siz);

	return siz;
#else
	return 0x7fffffff;
#endif
}



