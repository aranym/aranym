/*
	@(#)dosix/errno.h
	
	Julian F. Reschke, 19. Februar 1994
*/

extern int errno;

#define E_OK	0			/* OK (no error) */
#define ERROR	-1			/* Error */
#define EIO		ERROR
#define EDRVNR	-2			/* Drive not ready */
#define EUNCMD	-3			/* Unknown command */
#define E_CRC	-4			/* CRC error */
#define EBADRQ	-5			/* Bad request */
#define E_SEEK	-6			/* Seek error */
#define EMEDIA	-7			/* Unknown media */
#define ESECNF	-8			/* Sector not found */
#define EPAPER	-9			/* Out of paper */
#define EWRITF	-10			/* Write fault */
#define EREADF	-11			/* Read fault */
#define EGENRL	-12			/* General error */
#define EWRPRO	-13			/* Write on write-protected media */
#define EROFS	EWRPRO
#define E_CHNG	-14			/* Media change detected */
#define EAGAIN	E_CHNG
#define EUNDEV	-15			/* Unknown device */
#define EBADSF	-16			/* Bad sectors on format */
#define EOTHER	-17			/* Insert other disk (request) */
#define EINSERT	-18			/* Insert disk */
#define EDVNRSP	-19			/* Device not responding */
#define EINVFN	-32			/* Invalid function number */
#define EFILNF	-33			/* File not found */
#define ENOENT	EFILNF
#define ERSCH	EFILNF
#define EPTHNF	-34			/* Path not found */
#define ENOTDIR	EPTHNF
#define ENHNDL	-35			/* Handle pool exhausted */
#define EACCDN	-36			/* Access denied */
#define EACCES	EACCDN
#define EEXIST	EACCDN
#define EIHNDL	-37			/* Invalid handle */
#define ENSMEM	-39			/* Insufficient memory */
#define ENOMEM	ENSMEM
#define EIMBA	-40			/* Invalid memory block address */
#define EDRIVE	-46			/* Invalid drive specification */
#define ENSAME	-48			/* Not the same drive */
#define ENMFIL	-49			/* No more files */
#define ERLCKD	-58			/* Record is locked */
#define EMLNF	-59			/* Matching lock not found */
#define ERANGE	-64			/* Range error */
#define EINTRN	-65			/* GEMDOS internal error */
#define EPLFMT	-66			/* Invalid executable file format */
#define ENOEXEC	EPLFMT
#define EGSBF	-67			/* Memory block growth failure */
#define EBREAK	-68
#define EXCPT 	-69			/* Mag!x */
#define EPTHOV	-70
