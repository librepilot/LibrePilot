#
# Copyright (C) 2015, The LibrePilot Team, http://www.librepilot.org
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

# Make sure we know few things about the architecture before including
# the tools.mk to ensure that we download/install the right tools.
UNAME := $(shell uname)
# Here and everywhere if not Linux or Mac then assume Windows
ifeq ($(filter Linux Darwin, $(UNAME)), )
    UNAME := Windows
    CC := gcc# because cc doesn't exist
endif

ifeq ($(UNAME),Windows)
    system_path = $(shell cygpath -w $(1))
else
    system_path = $(1)
endif

# Function for converting Windows style slashes into Unix style
slashfix = $(subst \,/,$(1))

# Function for converting an absolute path to one relative
# to the top of the source tree
toprel = $(subst $(realpath $(ROOT_DIR))/,,$(abspath $(1)))

# Function to convert to all lowercase
lc = $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))

# Function to make all lowercase and replace spaces with -
EMPTY             :=
SPACE             := $(EMPTY) $(EMPTY)
define NEWLINE    :=


endef

smallify = $(subst $(SPACE),-,$(call lc,$1))

get_arch = $(shell $(CC) -dumpmachine | sed s/-.*//)
