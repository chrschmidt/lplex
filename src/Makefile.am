## Process this file with automake to produce Makefile.in

AM_CFLAGS = \
	$(LPLEX_CPPFLAGS) $(WX_CPPFLAGS) $(LIBFLAC_CFLAGS) $(LIBFLACPP_CFLAGS) \
		-w -Wall

AM_CXXFLAGS = \
	$(LPLEX_CPPFLAGS) $(WX_CPPFLAGS) $(LIBFLAC_CFLAGS) $(LIBFLACPP_CFLAGS) \
		-w -Wall

AM_CPPFLAGS =

bin_PROGRAMS = lplex

if HAVE_WINDRES
XTRA_SRC = desktop/lplex.rc
else
XTRA_SRC =
endif

lplex_SOURCES = \
	lplex.cpp \
	lplex.hpp \
	dvd.cpp \
	dvd.hpp \
	flac.cpp \
	flac.hpp \
	layout.cpp \
	lpcm.cpp \
	lpcm.hpp \
	main.cpp \
	platform.h \
	processor.hpp \
	reader.cpp \
	util.cpp \
	util.h \
	exec.cpp \
	video.cpp \
	writer.cpp \
	wx.cpp \
	wx.hpp \
	lgzip.cpp \
	color.h $(XTRA_SRC)

lplex_LDFLAGS = \
	$(WX_LIBS) $(LIBFLAC_LIBS) $(LIBFLACPP_LIBS)

lplex_LDADD = \
	$(top_builddir)/redist/md5/libmd5.a

if HAVE_WINDRES
.rc.o:
	$(WINDRES) $< -o $@
endif

