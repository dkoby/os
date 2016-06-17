#ifndef OS_VERSION_H
#define OS_VERSION_H

#define OS_VERSION_MAJOR  0
#define OS_VERSION_MINOR  0
#define OS_VERSION_BUILD  1

#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)

#define VERSION_STRING QUOTEME(MAJOR.MINOR.BUILD)

#endif

/*
 * CHANGELOG
 *
 * 20130912 v0.0.1
 *     * start of version tracking
 *
 */

