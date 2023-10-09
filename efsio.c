#include "include.h"
#include <time.h>
#include "efsio.h"

//========================================================
// Procedures for working with EFS files
// through the diagnostic interface
//========================================================

// Storage of the last error code
static int efs_errno;

// Flag indicating work with an alternative EFS
unsigned int efs_altflag = 0;   // flag for alternative EFS

//****************************************************
//* Get errno
//****************************************************
int efs_get_errno() {
  return efs_errno;
}

//****************************************************
//* Set alternative flag
//****************************************************
void set_altflag(int val) {
 efs_altflag = val;
}

//****************************************************
//* Send EFS command
//
// cmd - EFS_DIAG_* command code
// reqbuf - command parameters structure
// reqlen - length of the parameters structure
// respbuf - response structure
//
// returns the length of the response, or -1 on error
//****************************************************
int send_efs_cmd(int cmd, void* reqbuf, int reqlen, void* respbuf) {

char cmdbuf[200] = {0x4b, 0x13, 0, 0};
int iolen;
char iobuf[4096];

cmdbuf[2] = cmd;
if (efs_altflag) cmdbuf[1] = 0x3e;
if (reqbuf != 0) memcpy(cmdbuf + 4, reqbuf, reqlen);
iolen = send_cmd_base((unsigned char*)cmdbuf, reqlen + 4, iobuf, 0);
if (iolen == 0) {
  efs_errno = 9998;
  return -1;
}
if (iobuf[0] != 0x4b) {
  efs_errno = 9999;
  return -1;
}
//printf("\n cmd %02x:  resplen=%i\n",cmd,iolen-7);
//dump(iobuf+4,iolen-7,0);
memcpy(respbuf, iobuf + 4, iolen - 7);
return iolen; // no errors
}


//****************************************************
//* Get file description block by name
//*
//* Return value - file type:
//*
//* -1 - command processing error
//*  0 - file not found
//*  1 - file is not a directory
//*  2 - directory
//*
//****************************************************
int efs_stat(char* filename, struct efs_filestat* fi) {

unsigned char cmdbuf[200];
int iolen;

memset(cmdbuf, 0, 200);
strcpy(cmdbuf, filename);
iolen = send_efs_cmd(EFS2_DIAG_STAT, cmdbuf, strlen(filename) + 1, fi);
if (iolen == -1) return -1; // error receiving response
efs_errno = fi->diag_errno;
if (efs_errno != 0) return 0; // file not found or other error
if (S_ISDIR(fi->mode)) return 2;
return 1;
}

//****************************************************
//* Open directory for reading
//*
//* return:
//*  directory pointer
//*  0 - error
//****************************************************
int efs_opendir(char* path) {

unsigned char cmdbuf[200];
int iolen;
struct {
  uint32 dirp;             /* Directory pointer. NULL if error             */
  int32 diag_errno;        /* Error code if error, 0 otherwise             */
} rsp;

efs_errno = 0;
memset(cmdbuf, 0, 200);
strcpy(cmdbuf, path);
iolen = send_efs_cmd(EFS2_DIAG_OPENDIR, cmdbuf, strlen(path) + 1, &rsp);
if (iolen == -1) return 0;
efs_errno = rsp.diag_errno;
return rsp.dirp;
}


//****************************************************
//* Close directory
//****************************************************
int efs_closedir(int dirp) {

int ldirp = dirp;
int iolen;
int rsp;

iolen = send_efs_cmd(EFS2_DIAG_CLOSEDIR, &ldirp, 4, &rsp);
if (iolen == -1) return -1;
efs_errno = rsp;
return rsp;
}

//****************************************************
//* Read next directory entry
//****************************************************
int efs_readdir(int dirp, int seq, struct efs_dirent* rsp) {

// request structure
struct {
  uint32 dirp;             /* Directory pointer.                           */
  int32 seqno;             /* Sequence number of directory entry to read   */
} req;

int iolen;

req.dirp = dirp;
req.seqno = seq;
iolen = send_efs_cmd(EFS2_DIAG_READDIR, &req, sizeof(req), rsp);
if (iolen == -1) return -1;
efs_errno = rsp->diag_errno;
//printf("\n rsp: dirp=%i seq=%i entry=%i mode=%08x name=%s",rsp->dirp,rsp->seqno,rsp->entry_type,rsp->mode,rsp->name); fflush(stdout);
return efs_errno;
}

//**************************************************
//* Open EFS file
//*
//* The following oflag values are valid:
//* O_RDONLY (open for reading mode)
//* O_WRONLY (open for writing mode)
//* O_RDWR   (open for reading and writing)
//* O_TRUNC  (if successfully opened, truncate length to 0)
//* O_CREAT  (create file if it does not exist)
//*
//* The O_CREAT flag can be orred with the other flags.
//* The mode field is ignored unless the O_CREAT flag is specified.
//* If O_CREAT is specified, the mode is a three-digit octal number with
//* each octet representing read, write and execute permissions for owner,
//* group and other.
//**************************************************
int efs_open(char* filename, int oflag) {

struct {
  int32 oflag;             /* Open flags                                   */
  int32 mode;              /* Mode                                         */
  char  name[100];         /* Pathname (null-terminated string)            */
} req;

struct {
  int32 fd;                /* File descriptor if successful, -1 otherwise  */
  int32 diag_errno;        /* Error code if error, 0 otherwise             */
} rsp;

int iolen;

strcpy(req.name, filename);
req.oflag = oflag;
req.mode = 0777;
iolen = send_efs_cmd(EFS2_DIAG_OPEN, &req, sizeof(req), &rsp);
if (iolen == -1) return -1;
efs_errno = rsp.diag_errno;
return rsp.fd;
}

//**************************************************
//* Read EFS file
//*
//**************************************************
int efs_read(int fd, char* buf, int size, int offset) {

int iolen;
struct {
  int32  fd;               /* File descriptor                              */
  uint32 nbyte;            /* Number of bytes to read                      */
  uint32 offset;           /* Offset in bytes from the origin              */
} req;

struct {
  int32  fd;               /* File descriptor                              */
  uint32 offset;           /* Requested offset in bytes from the origin    */
  int32  bytes_read;       /* bytes read if successful, -1 otherwise       */
  int32  diag_errno;       /* Error code if error, 0 otherwise             */
  char   data[2048];       /* The data read out                            */
} rsp;

req.fd = fd;
req.nbyte = size;
req.offset = offset;
iolen = send_efs_cmd(EFS2_DIAG_READ, &req, sizeof(req), &rsp);
if (iolen == -1) return -1;
efs_errno = rsp.diag_errno;
if (rsp.bytes_read <= 0) return -1;
memcpy(buf, rsp.data, rsp.bytes_read);
return rsp.bytes_read;
}

//**************************************************
//* Close file
//**************************************************
int efs_close(int fd) {

int lfd = fd;
int lerrno;
int iolen;

iolen = send_efs_cmd(EFS2_DIAG_CLOSE, &lfd, 4, &lerrno);
if (iolen == -1) return -1;
efs_errno = lerrno;
return lerrno;
}


//**************************************************
//* Write file
//**************************************************
int efs_write(int fd, char* buf, int size, int offset) {

struct {
  int32 fd;                /* File descriptor                              */
  uint32 offset;           /* Offset in bytes from the origin              */
  char data[8192];        /* The data to be written                       */
} req;                    /* The size of data[] - "random".               */
                           /* Apparently, here should be the max file size of EFS */
struct  {
  int32 fd;                /* File descriptor                              */
  uint32 offset;           /* Requested offset in bytes from the origin    */
  int32 bytes_written;     /* The number of bytes written                  */
  int32 diag_errno;        /* Error code if error, 0 otherwise             */
} rsp;
int iolen;

req.fd = fd;
req.offset = offset;
memcpy(req.data, buf, size);
iolen = send_efs_cmd(EFS2_DIAG_WRITE, &req, 8 + size, &rsp);
if (iolen == -1) return -1;
efs_errno = rsp.diag_errno;
return rsp.bytes_written;
}


//******************************************************
//* Delete directory
//******************************************************
int efs_rmdir(char* dirname) {

int iolen;
int lerrno;

iolen = send_efs_cmd(EFS2_DIAG_RMDIR, dirname, strlen(dirname) + 1, &lerrno);
if (iolen == -1) return -1;
efs_errno = lerrno;
return lerrno;
}

//******************************************************
//* Delete file by name
//******************************************************
int efs_unlink(char* name) {

int iolen;
int lerrno;

iolen = send_efs_cmd(EFS2_DIAG_UNLINK, name, strlen(name) + 1, &lerrno);
if (iolen == -1) return -1;
efs_errno = lerrno;
return lerrno;
}

//******************************************************
//* Create directory
//******************************************************
int efs_mkdir(char* name, int mode) {

int iolen;
int lerrno;

struct {
  int16 mode;              /* The creation mode                            */
  char  name[100];         /* Pathname (null-terminated string)            */
} req;

req.mode = mode;
strcpy(req.name, name);
iolen = send_efs_cmd(EFS2_DIAG_MKDIR, &req, strlen(name) + 3, &lerrno);
if (iolen == -1) return -1;
efs_errno = lerrno;
return lerrno;
}

//******************************************************
//* Prepare for a full EFS dump
//******************************************************
int efs_prep_factimage() {

int iolen;
int lerrno;

iolen = send_efs_cmd(EFS2_DIAG_PREP_FACT_IMAGE, 0, 0, &lerrno);
if (iolen == -1) return -1;
efs_errno = lerrno;
return lerrno;
}

//******************************************************
//* Start reading the full EFS dump
//******************************************************
int efs_factimage_start() {

int iolen;
int lerrno;

iolen = send_efs_cmd(EFS2_DIAG_FACT_IMAGE_START, 0, 0, &lerrno);
if (iolen == -1) return -1;
efs_errno = lerrno;
return lerrno;
}

//******************************************************
//* Read the next segment of EFS
//******************************************************
int efs_factimage_read(int state, int sent, int map, int data, struct efs_factimage_rsp* rsp) {

struct {
  int8 stream_state;        /* Initialize to 0 */
  int8 info_cluster_sent;   /* Initialize to 0 */
  int16 cluster_map_seqno;  /* Initialize to 0 */
  int32 cluster_data_seqno; /* Initialize to 0 */
} req;

int iolen;

req.stream_state = state;
req.info_cluster_sent = sent;
req.cluster_map_seqno = map;
req.cluster_data_seqno = data;
iolen = send_efs_cmd(EFS2_DIAG_FACT_IMAGE_READ, &req, sizeof(req), rsp);
if (iolen == -1) return -1;
efs_errno = rsp->diag_errno;
return efs_errno;
}

//******************************************************
//* Finish the EFS dump
//******************************************************
int efs_factimage_end() {

int iolen;
int lerrno;

iolen = send_efs_cmd(EFS2_DIAG_FACT_IMAGE_END, 0, 0, &lerrno);
if (iolen == -1) return -1;
efs_errno = lerrno;
return lerrno;
}
