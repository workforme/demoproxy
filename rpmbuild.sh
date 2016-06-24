set -x

name=ldapproxy
pdir=$(pwd)"/../"
rpmbuild="${pdir}/rpmbuild/"

ver=${name}-2.3
rel=0
spec="${name}.spec"

find ${rpmbuild} -type f|xargs -i rm -rf {} &&
cd .. && mkdir -p rpmbuild && cd rpmbuild && rpmbuild -ba ../ldapproxy2/ldapproxy.spec  &>/dev/null 
cd ../
[ -d rpmbuild ] || exit

cp -rf ldapproxy2 $ver &&
tar -zcf ${rpmbuild}/SOURCES/${ver}.tar.gz $ver &&
cp -rf ${ver}/$spec ${rpmbuild}/SPECS/  &&

cd ${rpmbuild}/SPECS/ &&
rpmbuild -ba ${spec}  &&

rsync -av  ${rpmbuild}/RPMS/x86_64/${ver}-${rel}.x86_64.rpm  xianguo@172.16.15.148::entsmail_rpm &&

rm -rf ${pdir}"/"${ver}  &&

echo -e "DOWNLOAD LDAPPROXY USING:\nrsync 172.16.15.148::entsmail_rpm/${ver}-${rel}.x86_64.rpm ."

