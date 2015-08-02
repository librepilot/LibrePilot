
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
smallify = $(subst $(SPACE),-,$(call lc,$1))

