%define name ldapproxy
%define version 2.3
%define release 0
%define _prefix /usr/local/ldapproxy/ 

Summary: LdapProxy  Refer http://wiki.intra.sina.com.cn/display/mail/LdapProxy
Name: %{name}
Version: %{version}
Release: %{release}
License: GPL 
Group: Networking/Daemons
Source: %{name}-%{version}.tar.gz
Distribution: RHEL6.5
Vendor: sina, Inc.
Packager: yilong@staff.sina.com.cn
AutoReqProv: no
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description

%prep
%setup -q

%build
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
make

%install
rm -rf %{buildroot}
mkdir -p  %{buildroot}/%{_prefix}

cp ./run %{buildroot}/%{_prefix}/run
cp -rf ./extra-lib     %{buildroot}/%{_prefix}/extra-lib
install -s -m 0755 ./ldapproxy      %{buildroot}/%{_prefix}/ldapproxy
install -m 0644    ./ldapproxy.ini  %{buildroot}/%{_prefix}/ldapproxy.ini 

%post
echo "install  %{name}-%{version} success ..."

#rpm -e执行后
%postun
rm -rf %{_prefix}

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_prefix}/*
