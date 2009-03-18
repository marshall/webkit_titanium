if [ `uname -m` = "x86_64" ]
then
 ARCH="linux64"
else
 ARCH="linux"
fi
cp .libs/libwebkit-1.0.so.1.0.0 ../titanium/kroll/thirdparty/$ARCH/webkit/lib
cp libwebkit-1.0.la ../titanium/kroll/thirdparty/$ARCH/webkit/lib
