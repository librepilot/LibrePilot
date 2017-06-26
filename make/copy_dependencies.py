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
# ntldd seems to be more reliable but is not recursive (so recursion needs to be done here)
def ldd(file):
	ldd_output = subprocess.check_output(["ntldd", file])
	# sanitize output
	ldd_output = ldd_output.strip()
	ldd_output = ldd_output.replace('\\', '/')
	# split output into lines
	ldd_lines = ldd_output.split('\n')
	# parse lines that match this format : <file name> ==> <file path> (<memory address>)
	file_pattern = ".*/mingw../bin/.*"
	pattern = "(.*) => (" + file_pattern + ") \((.*)\)";
	regex = re.compile(pattern)
	dependencies = {m.group(2).strip() for m in [regex.match(line) for line in ldd_lines] if m}
	return dependencies

def find_dependencies(file, visited):
	print "Analyzing " + file
	visited.add(file)
	dependencies = ldd(file)
	# recurse
	for f in dependencies.copy():
		if f in visited:
			continue
		if args.excludes and os.path.basename(f) in args.excludes:
			print "Ignoring " + f
			dependencies.remove(f)
			continue
		dependencies = dependencies | find_dependencies(f, visited)
	return dependencies

parser = argparse.ArgumentParser(description='Find and copy dependencies to a destination directory.')
parser.add_argument('-n', '--no-copy', action='store_true', help='don\'t copy dependencies to destination dir')
parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose mode')
parser.add_argument('-d', '--dest', type=str, default='.', help='destination directory (defaults to current directory)')
parser.add_argument('-s', '--source', type=str, default='.', help='source directory (defaults to current directory)')
parser.add_argument('-f', '--files', metavar='FILE', nargs='+', required=True, help='a file')
parser.add_argument('-e', '--excludes', metavar='FILE', nargs='+', help='a file')

args = parser.parse_args()

# check that args.dest exists and is a directory
if not os.path.isdir(args.dest):
	print "Error: destination " + str(args.dest) + " is not a directory."
	exit(1)

# find dependencies
dependencies = set()
visited = set()
for file in args.files:
	file = os.path.join(args.source, file)
	files = glob.glob(file)
	for f in files:
		dependencies = dependencies | find_dependencies(f, visited)
print "Found " + str(len(dependencies)) + " dependencies."

# no copy, exit now
if args.no_copy:
	exit(0)

# copy dependencies to destination dir
copy_count = 0
for file in dependencies:
	dest_file = args.dest + "/" + os.path.basename(file)
	if not os.path.isfile(dest_file):
		print "Copying " + file + " to " + args.dest
		shutil.copy2(file, args.dest)
		copy_count += 1
print "Copied " + str(copy_count) + " dependencies to '" + str(args.dest) + "'."

exit(0)
