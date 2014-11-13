# TODO: broken on archs with sizeof(int)!=sizeof(void*) (i.e. all 64-bit)
#       (it abuses guint field to place strings - see gr_gtk.c:670 and below)
Name:		xoscope
Version:	2.0
Release:	0.2%{?dist}
#BuildArch:	i686
License:	GPL
Group:		X11/Applications
URL:		http://xoscope.sourceforge.net/
Vendor:		Brent Baccala & others
Packager:	Franta Hanzlik
Summary:	xoscope - digital oscilloscope on PC
Summary(pl.UTF-8):	xoscope - cyfrowy oscyloskop na PC
Source0:	http://dl.sourceforge.net/xoscope/%{name}-%{version}.tgz
Source1:	xoscope.desktop
Source2:	xoscope.png
Source3:	xoscope-at-fedora
Source4:	xoscope-screenshot_23.7.2012_09:09:47.png
Source5:	xoscope-screenshot_23.7.2012_09:10:42.png
Source6:	xoscope-screenshot_23.7.2012_09:10:59.png
Source7:	xoscope-screenshot_23.7.2012_09:21:01.png
Source8:	xoscope-screenshot_23.7.2012_09:23:39.png
Source9:	xoscope-screenshot_23.7.2012_09:24:01.png
Source10:	xoscope-screenshot_23.7.2012_09:24:10.png
Source11:	xoscope-screenshot_23.7.2012_09:41:57.png
Source12:	xoscope-screenshot_23.7.2012_09:50:02.png
Patch0:		xoscope-Makefile.am.patch
Patch1:		xoscope-comedi.patch
Patch2:		xoscope-gtkdatabox.c.patch
BuildRequires:	autoconf
BuildRequires:	automake
BuildRequires:	esound-devel
BuildRequires:	gtk2-devel >= 2.18
BuildRequires:	libtool
BuildRequires:	perl-devel
BuildRequires:	comedilib-devel
# xoscope se musi spoustet pomoci padsp wrapperu, tedy:
Requires:	pulseaudio-utils
Requires:	pulseaudio-esound-compat
BuildRoot:	%{tmpdir}/%{name}-%{version}-root-%(id -u -n)
 
%description
x*oscope is a digital oscilloscope that uses a sound card (via
/dev/dsp or EsounD) and/or Radio Shack ProbeScope a.k.a osziFOX as the
signal input.
 
%description -l pl.UTF-8
x*oscope jest to cyfrowy oscyloskop, który używa jako sygnału
wejściowego karty dźwiękowej (/dev/dsp albo EsounD) i/lub Radio Shack
ProbeScope znanego także jako osziFOX.
 
%description -l cz.UTF-8
xoscope je digitální osciloskop, který užívá pro vstup signálu zvukovou kartu
(přes /dev/dsp nebo EsounD) a/nebo Radio Shack ProbeScope (známého též jako
osziFOX).
 
%prep
%setup -q
%patch0 -p1
%patch1 -p1
%patch2 -p1
 
%build
%{__aclocal}
%{__libtoolize}
%{__autoheader}
%{__automake}
%{__autoconf}
%configure
%{__make}
 
%install
rm -rf $RPM_BUILD_ROOT
 
%{__make} install DESTDIR=$RPM_BUILD_ROOT

desktop-file-install --vendor=fedora --dir=${RPM_BUILD_ROOT}%{_datadir}/applications %{SOURCE1}

mkdir -p ${RPM_BUILD_ROOT}%{_datadir}/pixmaps
install -p -m 0644 %SOURCE2 ${RPM_BUILD_ROOT}%{_datadir}/pixmaps
install -d -p -m 0755 ${RPM_BUILD_ROOT}%{_docdir}/%{name}-%{version}/contrib
install -p -m 644 -t ${RPM_BUILD_ROOT}%{_docdir}/%{name}-%{version}/contrib %SOURCE3
install -p -m 644 -t ${RPM_BUILD_ROOT}%{_docdir}/%{name}-%{version}/contrib %{SOURCE4}\
 %{SOURCE5} %{SOURCE6} %{SOURCE7} %{SOURCE8} %{SOURCE9} %{SOURCE10} %{SOURCE11} %{SOURCE12}
 
%clean
rm -rf $RPM_BUILD_ROOT
 
%files
%defattr(644,root,root,755)
%doc AUTHORS ChangeLog hardware NEWS README TODO
%attr(755,root,root) %{_bindir}/*
%{_mandir}/*/*
%{_datadir}/pixmaps/*
%{_datadir}/applications/*
 
%define date	%(echo `LC_ALL="C" date +"%a %b %d %Y"`)

%changelog
* Sun Jul 22 2012 Franta Hanzlik <franra@hanzlici.cz>
- build for Fedora distros
- patch for nonexistent asm/page.h and no. of comedi_get_cmd_generic_timed params in comedi.c
- disable CFLAGS 'GTK_DISABLE_DEPRECATED' in gtkdatabox-0.6.0.0/gtk/Makefile.am
- added .desktop file
- added notes for xoscope on Fedora distro

* Tue Jul 20 2010 UTC PLD Team <feedback@pld-linux.org>
Revision 1.9  2010/07/20 14:01:17  draenog
- up to 2.0
- remove needless pmake.patch
- add patch for TK_WIDGET_STATE undefined error
  (taken from Debian)
 
Revision 1.8  2010/01/01 22:33:08  sparky
- BR capitalization
 
Revision 1.7  2007/02/12 22:09:25  glen
- tabs in preamble
 
Revision 1.6  2007/02/12 01:06:40  baggins
- converted to UTF-8
 
Revision 1.5  2005/08/03 18:52:26  qboosh
- better errors explanation
 
Revision 1.4  2005/08/03 18:38:12  qboosh
- pl fixes, cosmetics
 
Revision 1.3  2005/08/01 15:45:03  witekfl
- added pmake.patch
- rel 0.2
 
Revision 1.2  2005/07/31 21:50:34  glen
- fix for multiple make jobs failing build
- doesn't build on amd64
 
Revision 1.1  2005/07/31 21:39:43  havner
- by Jacek 'jackass' Brzozowski <metallowiec@op.pl>
- BR fixes
