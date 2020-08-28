Name:           fort
Version:        1.4.0
Release:        1%{?dist}
Summary:        RPKI certificate path validator and RTR server

License:        BSD
URL:            https://github.com/NICMx/FORT-validator
Source0:        https://github.com/NICMx/FORT-validator/archive/fort-%{version}.tar.gz
Source1:        https://github.com/NICMx/FORT-validator/blob/master/fort_setup.sh
Source2:        fort.service
Source3:        malloc.conf
Source4:        config.json
Source5:        fort.logrotate
Source6:        fort.rsyslog

Requires:       logrotate
Requires:       rsyslog
Requires:       rsync
#
# fort setup.sh script needs to be rewritten to use curl which is
# installed in CentOS by default instead of wget
#
Requires:       wget

%description
FORT Validator is a RPKI Validator and RTR Server, part of the FORT project (https://www.fortproject.net/).

%prep
%autosetup

%build
%configure
%make_build

%install
%make_install
%{__mkdir} -p $RPM_BUILD_ROOT%{_unitdir}
%{__mkdir} -p $RPM_BUILD_ROOT%{_sysconfdir}/fort
%{__mkdir} -p $RPM_BUILD_ROOT%{_sysconfdir}/logrotate.d
%{__mkdir} -p $RPM_BUILD_ROOT%{_sysconfdir}/rsyslog.d
%{__mkdir} -p $RPM_BUILD_ROOT%{_sysconfdir}/systemd/system/fort.service.d
%{__mkdir} -p $RPM_BUILD_ROOT%{_localstatedir}/{cache,log}/fort
%{__mkdir} -p $RPM_BUILD_ROOT%{_localstatedir}/cache/fort/{tal,repository}

# %{__install} -m644 examples/config.json $RPM_BUILD_ROOT%{_sysconfdir}/fort/config.json
%{__install} -m644 examples/demo.slurm $RPM_BUILD_ROOT%{_sysconfdir}/fort/fort.slurm
%{__install} -m644 examples/tal/*.tal $RPM_BUILD_ROOT%{_localstatedir}/cache/fort/tal
%{__install} -m644 %SOURCE1 $RPM_BUILD_ROOT%{_localstatedir}/cache/fort/fort_setup.sh
%{__install} -m644 %SOURCE2 $RPM_BUILD_ROOT%{_unitdir}/fort.service
%{__install} -m644 %SOURCE3 $RPM_BUILD_ROOT%{_sysconfdir}/systemd/system/fort.service.d/malloc.conf
%{__install} -m644 %SOURCE4 $RPM_BUILD_ROOT%{_sysconfdir}/fort/config.json
%{__install} -m644 %SOURCE5 $RPM_BUILD_ROOT%{_sysconfdir}/logrotate.d/fort
%{__install} -m644 %SOURCE6 $RPM_BUILD_ROOT%{_sysconfdir}/rsyslog.d/fort.conf

%clean
%{__rm} -rf $RPM_BUILD_ROOT

%files
%{_bindir}/fort
%{_mandir}/man8/fort.8*
%{_unitdir}/fort.service
%{_sysconfdir}/logrotate.d/fort
%{_sysconfdir}/rsyslog.d/fort.conf
%{_sysconfdir}/fort/config.json
%{_sysconfdir}/fort/fort.slurm
%{_sysconfdir}/systemd/system/fort.service.d/malloc.conf
%{_localstatedir}/log/fort
%{_localstatedir}/cache/fort/{tal,repository}
%{_localstatedir}/cache/fort/fort_setup.sh
%{_localstatedir}/cache/fort/tal/*.tal

%post
if [ $1 -eq 1 ]; then
        /usr/bin/systemctl preset fort.service >/dev/null 2>&1 ||:
        /usr/bin/systemctl restart syslog >/dev/null 2>&1 ||:
        echo "yes" | /bin/sh /var/cache/fort/fort_setup.sh /var/cache/fort/tal >/dev/null 2>&1 ||:
fi

%preun
/usr/bin/systemctl --no-reload disable fort.service >/dev/null 2>&1 ||:
/usr/bin/systemctl stop fort.service >/dev/null 2>&1 ||:

%changelog
* Thu Aug 27 2020 Volodymyr Pidgornyi <vp@dtel-ix.net> - 1.4.0-1
- Initial package for CentOS8.
