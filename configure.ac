AC_PREREQ(2.61)

###
# Version info
###
m4_define([lplex_version_major],    [0])
m4_define([lplex_version_minor],    [3])
m4_define([lplex_version_revision], [2])
m4_define([lplex_version], [lplex_version_major.lplex_version_minor.lplex_version_revision])

###
# Autotools Setup
###
AC_INIT([lplex],[lplex_version],,[lplex])
AC_CONFIG_SRCDIR([src/lplex.cpp])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADER([src/config.h])
AC_CANONICAL_HOST

###
# User-controled options
###
AC_ARG_ENABLE(tool-checks,     [  --disable-tool-checks        Don't test for presence of required runtime tools])
AC_ARG_ENABLE(colors,          [  --disable-colors             Don't color console output]
                               ,colors=no,colors=yes)
#AC_ARG_ENABLE(build-win32,     [  --enable-build-win32         Configure for msys/mingw32 win32 build]
#                               ,build_win32=yes,)
AC_ARG_ENABLE(pkg-win32,       [  --disable-pkg-win32          Don't install win32 files in a standalone
                               directory structure],pkg_win32=no,pkg_win32=yes)
AC_ARG_ENABLE(force-endian,    [  --enable-force-endian=x      Use x-endian byte-order, where x is
                               'big', 'little', or 'opp' (opposite of detected)], force_endian=$enableval, force_endian=no)

###
# Host Stuff
###
case "$host" in
	*-*-cygwin | *mingw*)
		build_win=yes
#		LPLEX_DEFS="-Dlplex_win32"
		AC_CHECK_TOOL(WINDRES, windres)
		;;
	*)
#		LPLEX_DEFS="-Dlplex_linux"
		AC_CHECK_PROG(XDGUTILS, xdg-mime, yes)
		pkg_win32=no
		;;
esac

AC_C_BIGENDIAN

# Checks for programs
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
PKG_PROG_PKG_CONFIG([0.23])
# FIXME Revisit after md5 removal
#AC_PROG_CC
#AC_PROG_RANLIB

###
# Checks for libraries
###

# Checks for header files
AC_CHECK_HEADERS([fcntl.h stdint.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for standard library functions
AC_CHECK_FUNCS([atexit memset putenv strrchr strstr])

# --------------------------------------------------------------------------
# --------------------------------------------------------------------------

AM_OPTIONS_WXCONFIG

AM_CONDITIONAL([MSW_RELEASE], [test "x$pkg_win32" = "xyes"])
AM_CONDITIONAL([HAVE_WINDRES], [test "x$WINDRES" != "x"])
AM_CONDITIONAL([HAVE_FREEDESKTOP], [test "x$XDGUTILS" = "xyes"])

if test "x$colors" = "xno" ; then
	LPLEX_DEFS="$LPLEX_DEFS -Dlplex_nocolor"
fi

#if test "x$build-console" = "xyes" ; then
#	LPLEX_DEFS="$LPLEX_DEFS -Dlplex_console"
#else
#	LPLEX_DEFS="$LPLEX_DEFS -Dlplex_gui"
#fi

#if test "x$force_async" = "xyes" ; then
#	LPLEX_DEFS="$LPLEX_DEFS -Dlplex_async"
#	echo "forcing asynchronous external process execution per your request"
#fi


# Checks for libraries.

# ensure '-ldl' is attached to link if system uses libdl,
# otherwise AC_CHECK_LIB tests will fail to load other shared objects
AC_CHECK_LIB(dl, dlopen, LIBDL="-ldl", LIBDL="")

# --------------------------------------------------------------------------
# check for libav
# --------------------------------------------------------------------------

PKG_CHECK_MODULES([libavutil],     [libavutil])
PKG_CHECK_MODULES([libavcodec],    [libavcodec])
PKG_CHECK_MODULES([libavformat],   [libavformat])
PKG_CHECK_MODULES([libavresample], [libavresample])

LIBAV_LIBS="$libavutil_LIBS $libavcodec_LIBS $libavformat_LIBS $libavresample_LIBS"
LIBAV_CFLAGS="$libavutil_CFLAGS $libavcodec_CFLAGS $libavformat_CFLAGS $libavresample_CFLAGS -D__STDC_CONSTANT_MACROS"
AC_SUBST(LIBAV_LIBS)
AC_SUBST(LIBAV_CFLAGS)


# --------------------------------------------------------------------------
# check for libjpeg
# --------------------------------------------------------------------------
#AC_CHECK_LIB(jpeg, jpeg_read_header,,
#  [AC_MSG_ERROR("Can't find libjpeg: is libjpeg-devel installed and on your path?")],
#)

# --------------------------------------------------------------------------
# check for libdvdread
# --------------------------------------------------------------------------
AC_CHECK_HEADER(dvdread/dvd_reader.h,dvdrd_h=yes,
  [AC_MSG_ERROR("Can't find libdvdread headers: are they on your path?")]
)
AC_CHECK_LIB(dvdread, ifoOpen,,
  [AC_MSG_ERROR("Can't find libdvdread: is it installed and on your path?")], $LIBDL
)
AC_CHECK_LIB(dvdread, udflist,
	LPLEX_DEFS="$LPLEX_DEFS -Ddvdread_udflist",, $LIBDL
)

# --------------------------------------------------------------------------
# configure libFLAC
# --------------------------------------------------------------------------
AM_PATH_LIBFLAC(,AC_MSG_ERROR([Can't find libFLAC]))
AC_LANG(C++)
AM_PATH_LIBFLACPP(,AC_MSG_ERROR([Can't find libFLAC++]))
if test "x$build_win32" = "xyes" ; then
	# to prevent ntoh() undefined reference
	LIBFLACPP_LIBS="$LIBFLACPP_LIBS -lwsock32"
	AC_SUBST(LIBFLACPP_LIBS)
fi

# remove any empty '-L ' flags in LIBS vars, which cause link failure
LIBFLAC_LIBS=$(echo $LIBFLAC_LIBS|sed -e 's/-L //g')
LIBFLACPP_LIBS=$(echo $LIBFLACPP_LIBS|sed -e 's/-L //g')
AC_SUBST(LIBFLAC_LIBS)
AC_SUBST(LIBFLACPP_LIBS)

# --------------------------------------------------------------------------
# configure wxWidgets
# --------------------------------------------------------------------------

wxFail=0
AM_PATH_WXCONFIG(2.7.2, ,wxFail=1,,)
if test "$wxFail" = 1; then
   AC_MSG_ERROR([
*   Can't find an appropriate version of wxWidgets. Please check if
*      - wx-config is in path.
*      - wxWidgets version is 2.7.2 or above.
*      - wxWidgets build is ANSI (not unicode).
   ])
fi

# wx-config defaults to gui link flags (-mwindows, etc) which block console io,
# so we need to redo WX_LIBS
if test "x$build_win32" != "x" ; then
   WX_LIBS="$(wx-config --libs core,base | sed -e 's/ -Wl,--subsystem,windows -mwindows//')"
#   WX_LIBS="$(echo $WX_LIBS | sed -e 's/ -mthreads//')"
   WX_LIBS="-Wl,--subsystem,console $WX_LIBS -s"
   WX_LIBS_STATIC=$WX_LIBS
	AC_SUBST(WX_LIBS)
	AC_SUBST(WX_LIBS_STATIC)
fi

# we assume that if wx-config is found, wxWindows is successfully installed.

#AC_PATH_PROG(WX_CONFIG_PATH, wx-config, no, $PATH:/usr/local/bin )
#if [[ "$WX_CONFIG_PATH" = "no" ]] ; then
#AC_MSG_ERROR("Couldn't find wx-config: is wx-config in your path?")
#fi

#AC_MSG_CHECKING([wxWidgets version])
#wx_version=`${WX_CONFIG_PATH}  --version`
#AC_MSG_RESULT($wx_version)


if test "x$build_win32" = "x" ; then
# --------------------------------------------------------------------------
# check how to run external tools
# --------------------------------------------------------------------------
   AC_CHECK_FUNCS(popen,havepopen=1,)
   if test "x$havepopen" = "x" ; then
      AC_PATH_PROG(SCRIPT_PATH, script)
      if test "x$SCRIPT_PATH" = "x" ; then
      	AC_MSG_WARN([	lplex requires either (preferably) the libc function popen()
      	at compile time, or as a fallback the bsd command 'script' at runtime.
      	Please check that at an absolute minimum 'script' is available.])
      else
      	AC_MSG_WARN([	Using 'script' as a fallback to popen(). Some
      	console output from the runtime tools will be delayed.])
      fi
   	LPLEX_DEFS="$LPLEX_DEFS -Dno_popen"
   fi
fi # !build_win32

if test "$enable_tool_checks" = "no" || test "x$build_win32" = "xyes"; then
   echo "skipping runtime tools check..."
else

   # 'read tok1 tok2 tok3 tok4 tok5 tok6 <<< $txt' isn't portable since
   # the '<<<' operator isn't supported in the default msys shell and
   # generates a syntax error causing configure to fail
   readtoks()
   {
      tok1=$1; tok2=$2; tok3=$3; tok4=$4; tok5=$5; tok6=$6
   }

# --------------------------------------------------------------------------
# check for runtime tools
# --------------------------------------------------------------------------
   echo "checking for presence of required runtime tools"

   AC_PATH_PROG(DVDAUTHOR_PATH, dvdauthor)
   if test "x$DVDAUTHOR_PATH" != "x" ; then
#   	AC_MSG_WARN([lplex requires dvdauthor >= 0.6.14 at runtime.])
#   else
   	AC_MSG_CHECKING([dvdauthor version])
   	readtoks $(dvdauthor 2>&1)
   	dvdauthorVer=$tok3
   	AC_MSG_RESULT([$dvdauthorVer])

   	readtoks $(echo "${dvdauthorVer}"|tr '[(.)]' ' ')
   	x=`echo $(($tok1*10000+$tok2*100+$tok3))`
   	if(( x<611 )) ; then
         AC_MSG_WARN(dvdauthor version $dvdauthorVer is too old.)
   	fi
   	if(( x<614 )) ; then
         echo "*   Please upgrade dvdauthor to version 0.6.14 or greater.
*   Although lplex will work correctly with dvdauthor 0.6.11
*   in most cases, dvd navigation may fail if timestamps
*   exceed approximately 2 hours for any given stream.  Always
*   ensure that the 'dvdauthor' section of lplex.log is free of
*   warnings prior to burning if using dvdauthor 0.6.11.
"
   	fi
   fi

   AC_PATH_PROG(MPLEX_PATH, mplex)
   if test "x$MPLEX_PATH" != "x" ; then
#    	AC_MSG_WARN([lplex requires (mjpegtools) mplex >= 2.2.5 at runtime.])
##	echo "*   You'll need to either apply a patch or get a current cvs update. Please see the
##*   readme in the 'patch' folder (included with this program)
##*   for more on how to do this.
##"
#   else
   	AC_MSG_CHECKING([mplex version])
   	readtoks $(mplex 2>&1)
   	mjpegVer=$tok4
   	mplexVer=$tok5
   	mplexVer=$(echo $mplexVer|tr -d '[()]')
   	AC_MSG_RESULT([$mplexVer (mjpegtools $mjpegVer)])

   	readtoks $(echo "${mplexVer}"|tr '.' ' ')
   	x=`echo $(($tok1*10000+$tok2*100+$tok3))`
   	if(( x<20205 )) ; then
         AC_MSG_WARN(mplex version number is too old.)
   	echo "*   The reported version of mplex $mplexVer predates a critical
*   fix for lpcm (in version 2.2.5), without which white noise and/or
*   unplayable dvds will result at some audio resolutions.  Please either
*   upgrade your mjpegtools package to >= 1.9.0, or see the readme in the
*   'patch' subfolder (included with this program) for more on how to fix
*   this version of mplex.
"
   	fi
   fi

   AC_PATH_PROG(JPEG2YUV_PATH, jpeg2yuv)
   AC_PATH_PROG(MPEG2ENC_PATH, mpeg2enc)
   AC_PATH_PROG(MKISOFS_PATH, mkisofs)

   missing=""

   if test "x$DVDAUTHOR_PATH" = "x" ; then
   	AC_MSG_WARN([lplex requires dvdauthor 0.6.14+ at runtime])
      missing="$missing dvdauthor"
   fi
   if test "x$MPLEX_PATH" = "x" ; then
    	AC_MSG_WARN([lplex requires mplex 2.2.5+ at runtime (pkg: mjpegtools 1.9.0+)])
      missing="$missing mplex"
   fi
   if test "x$JPEG2YUV_PATH" = "x" ; then
    	AC_MSG_WARN([lplex requires jpeg2yuv at runtime (pkg: mjpegtools)])
      missing="$missing jpeg2yuv"
   fi
   if test "x$MPEG2ENC_PATH" = "x" ; then
   	AC_MSG_WARN([lplex requires mpeg2enc at runtime (pkg: mjpegtools)])
      missing="$missing mpeg2enc"
   fi
   if test "x$MKISOFS_PATH" = "x" ; then
   	AC_MSG_WARN([lplex requires mkisofs at runtime (pkg: cdrtools)])
      missing="$missing mkisofs"
   fi

fi # tool_check

LPLEX_CPPFLAGS="$LPLEX_DEFS -I../redist"
case "$host" in
	(*-*-cygwin) LPLEX_CPPFLAGS="-mno-cygwin $LPLEX_CPPFLAGS";;
	(*) ;;
esac
AC_SUBST(LPLEX_CPPFLAGS)

#AC_CONFIG_AUX_DIR(directory)
#AC_PROG_INSTALL

x64=$(echo $host|grep 'x86_64')
if test "x$x64" != "x" ; then
    AC_DEFINE([BUILD_VER], ["-x64"], [64bit build indicator])
else
    AC_DEFINE([BUILD_VER], [""], [64bit build indicator])
fi
AC_DEFINE_UNQUOTED([BUILD_HOST], ["$host"], [build host])
AC_DEFINE_UNQUOTED([BUILD_DEFS], ["$LPLEX_DEFS"], [extra build flags])

AC_CONFIG_FILES([Makefile
                 data/Makefile
                 patch/Makefile
                 redist/Makefile
                 src/Makefile
                 src/desktop/Makefile
                 doc/Makefile
                 msw/Makefile
                 msw/util/Makefile])

AC_OUTPUT


echo "
configured for '$host'"

if test "x$LPLEX_DEFS" != "x" ; then
echo "   using $LPLEX_DEFS"
fi

if test "x$pkg_win32" = "xyes" ; then
#	DESTDIR=@top_builddir@/release/${PACKAGE}-${VERSION}
#	AC_SUBST(DESTDIR)
#	echo $DESTDIR
	echo "   installing as standalone release"
	missing="dvdauthor mplex jpeg2yuv mpeg2enc mkisofs"
fi
echo ""
if test "x$missing" != "x" ; then
   echo "Note: please install required runtime tools ($missing - see above) before running lplex."
fi
