# SPDX-License-Identifier: Apache-2.0
#
# Makefile to build documentation using Doxygen, Sphinx and Breathe

BUILDDIR       = build
SPHINXBUILD    = sphinx-build
SPHINXOPTS     = -j auto

SPHINX_INSTALLED := $(shell command -v $(SPHINXBUILD) 2> /dev/null)

ifndef SPHINX_INSTALLED
$(error '$(SPHINXBUILD)' executable was not found. Please install from https://www.sphinx-doc.org)
endif

ALLSPHINXOPTS   = -d $(BUILDDIR)/doctrees $(SPHINXOPTS) .

.PHONY: clean html

html:
	mkdir -p build/doxygen
	doxygen
	$(SPHINXBUILD) -b html $(ALLSPHINXOPTS) $(BUILDDIR)/html

clean:
	rm -rf $(BUILDDIR)/*
