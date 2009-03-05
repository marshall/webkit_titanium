#!/usr/bin/env python
import os, sys, re
import subprocess
import shutil

def uname_matches(n): return bool(re.match(os.uname()[0], n))
platform = None
if not hasattr(os, 'uname') or uname_matches('CYGWIN'):
	platform = 'win32'
elif uname_matches('Darwin'):
	platform = 'osx'
elif uname_matches('Linux'):
	platform = 'linux'

print "Build Webkit_Titanium (%s)..." % platform
try:
	if platform is 'linux':
		os.system("./configure")
		os.system("make")
	elif platform is 'osx':
		# TODO: add logic for 10.4/10.5 build
		os.system("./WebKitTools/Scripts/build-webkit --release")
	elif platform is 'win32':
		cygwin_dir = os.path.join('C:', 'cygwin')
		bash_exe = os.path.join(cygwin_dir, 'bin', 'bash.exe')
		cygpath_exe = os.path.join(cygwin_dir, 'bin', 'cygpath.exe')
		workspace_dir = os.environ("WORKSPACE")
		workspace_cyg_dir = subprocess.Popen([cygpath_exe, "-u", workspace_dir], stdout=PIPE).communicate()[0]
		subprocess.call([bash_exe, '-c', "%s/WebKitTools/Scripts/build-webkit" % workspace_cyg_dir, "--release", "--cairo-win32"])
		
except OSError, e:
	print "Error building: " + e