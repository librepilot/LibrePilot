#!/usr/bin/python2

 ###############################################################################
 #
 # The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 # Script to find and copy dependencies (msys2 only).
 # ./copy_dependencies.py -h for usage.
 #
 ###############################################################################

import argparse
import glob
import os
import re
import shutil
import subprocess
import sys

def cygpath(file):
	return subprocess.check_output(["cygpath", "-m", file]).strip()

# ldd does not work well on Windows 10 and is not supported on mingw32
def ldd(files):
	ldd_output = subprocess.check_output(["ntldd", "-R"] + files)
	# sanitize output
	ldd_output = ldd_output.strip()
	ldd_output = ldd_output.replace('\\', '/')
	# split output into lines
	ldd_lines = ldd_output.split(os.linesep)
	# parse lines that match this format : <file name> ==> <file path> (<memory address>)
	file_pattern = cygpath("/") + "mingw(32|64)/bin/.*"
	print file_pattern
	pattern = "(.*) => (" + file_pattern + ") \((.*)\)";
	regex = re.compile(pattern)
	dependencies = {m.group(2) for m in [regex.match(line.strip()) for line in ldd_lines] if m}
	return dependencies

parser = argparse.ArgumentParser(description='Find and copy dependencies to a destination directory.')
parser.add_argument('-n', '--no-copy', action='store_true', help='don\'t copy dependencies to destination dir')
parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose mode')
parser.add_argument('-d', '--dest', type=str, default='.', help='destination directory (defaults to current directory)')
parser.add_argument('-f', '--files', metavar='FILE', nargs='+', required=True, help='a file')
parser.add_argument('-e', '--excludes', metavar='FILE', nargs='+', help='a file')

args = parser.parse_args()

# check that args.dest exists and is a directory
if not os.path.isdir(args.dest):
	print "Error: destination " + str(args.dest) + " is not a directory"
	exit(1)

# find dependencies
dependencies = ldd(args.files)
print "Found " + str(len(dependencies)) + " new dependencies"

# no copy, exit now
if args.no_copy:
	exit(0)

# copy dependencies to destination dir
copy_count = 0
for file in dependencies:
	dest_file = args.dest + "/" + os.path.basename(file)
	if args.excludes and os.path.basename(file) in args.excludes:
		print "Ignoring " + file
		continue
	if not os.path.isfile(dest_file):
		print "Copying " + file + " to " + args.dest
		shutil.copy2(file, args.dest)
		copy_count += 1
print "Copied " + str(copy_count) + " dependencies to " + str(args.dest)

exit(0)
