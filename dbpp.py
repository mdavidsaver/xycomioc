#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
# dbpp.py is conceptually similar to the usual msi utility with
# several key differences.
#
# It uses the python string formatting syntax '%(macros)s'
# instead of the shell syntax '${macro}' and '$(macro)'.
# This allows the output to be used with dbLoadRecords('','')
# It can even be invoked recursively since a '%%' in the
# input will be translated to '%' in the output.
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

#
# Michael Davidsaver <mdavidsaver@bnl.gov>
# Brookhaven National Lab.
# 2009
#

import sys, re
from copy import deepcopy

if len(sys.argv)<2:
  print "Usage: dbpp.py <source file>"
  sys.exit(1)

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

def cleansep(inp):
  """Seperate a line of tokens or values
  and clean the individual values
  """
  x=inp.split(',')
  for n in range(len(x)):
    x[n]=x[n].strip()
  return x

def filerr(mesg):
  print >>sys.stderr, 'On line %(line)d of %(file)s: %(mesg)s'% \
    {'line':line,'file':sys.argv[1],'mesg':mesg}
  sys.exit(1)

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

    # quoted string variable
    svm=svar.match(l)
    if svm is not None:
      name, val = svm.groups()
      # globals can reference other existing globals
      globs[name]=val%globs
      continue

    # unset variable
    rvm=rvar.match(l)
    if rvm is not None:
      name = rvm.groups()[0]
      del globs[name]
      continue

    # open a new file
    fm=ifile.match(l)
    if fm is not None:
      infile=fm.groups()[0]
      xf=open(infile,'r')
      if infile in inpf:
        filerr('%s is already open'% infile)

      inpf[infile]=xf.read()
      xf.close()
      continue

    if len(inpf)==0:
      filerr('expected FILE: no files open')

    # close named file
    pm=pstop.match(l)
    if pm is not None:
      sfile=pm.groups()[0]

      if sfile not in inpf.keys():
        filerr('%s is not open'% sfile)

      del inpf[sfile]
      continue

    # close all open files
    if stop.match(l):
      inpf={}
      continue

    # token specification
    ptm=tokl.match(l)
    if ptm is not None:
      tokens=cleansep(ptm.groups()[0])
      continue

    # Must be a values line
    if tokens is None:
      filerr('expected TOKENS: no tokens specified before a values line')

    vals=cleansep(l)

    if len(tokens) != len(vals):
      filerr('the number of values (%(vals)d) does not match the number of tokens (%(toks)d)'% \
        {'vals':len(vals),'toks':len(tokens)}
      )

    # copy global macros for use in this values line
    macs=deepcopy(globs)
    # expand each value column in terms of the global macros
    # and those columns to its left.
    for n in range(len(tokens)):
      macs[tokens[n]]=vals[n]%macs

    # use macros to expand each file
    # and send to standard out.
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
