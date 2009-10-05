#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
# dbpp.py is conceptually similar the usual msi utility with
# several key differences.
#
# It uses the python string formatting syntax '%(macros)s'
# instead of the shell syntax '${macro}' and '$(macro)'.
# While it does not support all features of msi it can be
# invoked recursively since a '%%' in the input will be
# translated to '%' in the output.
#
# Template table file formatting
#
# '#' is a comment
# The first non-blank. non-comment line must contain the token
# 'FILE' followed by a quoted string with a filename.
# This filename may contain whitespace, but leading and trailing
# whitespace is ignored.
# The second line must be a list of token strings.
# Each successive line is a list of value strings which will
# be assigned to the token and used to expand the given
# input file(s)
# A column can be defined in terms of columns to its left.
# For example, for columns 'A' 'B' all values in column 'A'
# must be constants, but 'str%(A)s' is valid in column 'B'.
# Lines of the form 'token = "value"' set global tokens
# which may be used thereafter as if they were defined as
# part of a table.
#
# Input files
#
# There is no restriction on input files other then that
# all occurances of '%' be part of a valid python string
# formatter or escaped '%%'.
# References to undefined tokens will be passed through
# without modification.
#

import sys, re
from copy import deepcopy

if len(sys.argv)<2:
  print "Usage: dbpp.py <source file>"
  sys.exit(1)

def cleansep(inp):
  """Seperate a line of tokens or values
  and clean the individual values
  """
  x=inp.split(',')
  for n in range(len(x)):
    x[n]=x[n].strip()
  return x

comment=re.compile('^\W*#\W*')
empty=re.compile('^\W*$')

ifile=re.compile('\W*FILE\W*\"([^\"]*)\"')
stop=re.compile('\W*CLOSE\W*$')
pstop=re.compile('\W*CLOSE\W*\"([^\"]*)\"')

tokl=re.compile('\W*TOKENS\W*(.*)')
svar=re.compile('([a-zA-Z][a-zA-Z0-9]*)\W*=\W*\"([^\"]*)')
rvar=re.compile('\W*UNSET\W*([a-zA-Z][a-zA-Z0-9]*)')

# open template table file
f=open(sys.argv[1],'r')

tokens=None
line=0
inpf={}

globs={}

try:
  for l in f.readlines():
    #TODO: handle continuations '\$'

    line=line+1

    if empty.match(l):
      continue

    elif comment.match(l):
      print l,
      continue

    svm=svar.match(l)
    if svm is not None:
      name, val = svm.groups()
      globs[name]=val%globs
      continue

    rvm=rvar.match(l)
    if rvm is not None:
      name = svm.groups()
      del globs[name]
      continue

    fm=ifile.match(l)
    if fm is not None:
      infile=fm.groups()[0]
      xf=open(infile,'r')
      if infile in inpf:
        print >>sys.stderr, 'On line %(line)d of %(file)s: %(ifile)s is already open'% \
          {'line':line,'file':sys.argv[1],'ifile':infile}
        sys.exit(1)
      inpf[infile]=xf.read()
      xf.close()
      tokens=None
      continue

    if len(inpf)==0:
      print >>sys.stderr, 'On line %(line)d of %(file)s: expected FILE'% \
        {'line':line,'file':sys.argv[1]}
      sys.exit(1)

    pm=pstop.match(l)
    if pm is not None:
      sfile=fm.groups()
      if sfile not in inpf:
        print >>sys.stderr, 'On line %(line)d of %(file)s: %(ifile)s is not open'% \
          {'line':line,'file':sys.argv[1],'ifile':sfile}
        sys.exit(1)
      del inpf[sfile]
      continue

    if stop.match(l):
      inpf={}
      continue

    ptm=tokl.match(l)
    if ptm is not None:
      tokens=cleansep(ptm.groups()[0])
      continue

    # Must be a values line
    if tokens is None:
      print >>sys.stderr, 'On line %(line)d of %(file)s: expected TOKENS'% \
        {'line':line,'file':sys.argv[1]}
      sys.exit(1)

    vals=cleansep(l)

    if len(tokens) != len(vals):
      print >>sys.stderr, 'On line %(line)d of %(file)s: the number of values (%(vals)d) does not match the number of tokens (%(toks)d)'% \
        {'line':line,'file':sys.argv[1],'vals':len(vals),'toks':len(tokens)}
      sys.exit(1)

    macs=deepcopy(globs)
    for n in range(len(tokens)):
      macs[tokens[n]]=vals[n]%macs

    for fn,out in inpf.iteritems():
      infile=fn
      while True:
        try:
          sys.stdout.write(out%macs)
          break
        except KeyError,e:
          e=str(e)[1:-1] # strip quotes
          macs[e]='%%(%s)s'%e

except Exception,e:
  raise
  print >>sys.stderr, 'Error expanding',fn,':',e,'\n'
  print >>sys.stderr, "Are all '%' escaped? (ie '%%')"
  print >>sys.stderr, "All tokens must be complete. (ie %(name)s)"

f.close()

for opf in inpf:
  print >>sys.stderr, opf,"is still open"
