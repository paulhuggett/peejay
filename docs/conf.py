# ===- docs/conf.py -------------------------------------------------------===//
# *                   __  *
# *   ___ ___  _ __  / _| *
# *  / __/ _ \| '_ \| |_  *
# * | (_| (_) | | | |  _| *
# *  \___\___/|_| |_|_|   *
# *                       *
# ===----------------------------------------------------------------------===//
#  Copyright © 2026 Paul Bowen-Huggett
#
#  Permission is hereby granted, free of charge, to any person obtaining
#  a copy of this software and associated documentation files (the
#  “Software”), to deal in the Software without restriction, including
#  without limitation the rights to use, copy, modify, merge, publish,
#  distribute, sublicense, and/or sell copies of the Software, and to
#  permit persons to whom the Software is furnished to do so, subject to
#  the following conditions:
#
#  The above copyright notice and this permission notice shall be
#  included in all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
#  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
#  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
#  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
#  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
#  SPDX-License-Identifier: MIT
# ===----------------------------------------------------------------------===//

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import os
import pathlib
import subprocess

project = 'peejay'
copyright = '2026, Paul Bowen-Huggett'
author = 'Paul Bowen-Huggett'
version = '0.1'
release = version

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
     'breathe',
     'sphinx.ext.autosectionlabel',
     'sphinx.ext.graphviz',
     'sphinx_design'
]

# Breathe configuration
breathe_projects = { project: './xml/' }
breathe_default_project = project

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'furo'
html_static_path = ['_static']

if os.environ.get('READTHEDOCS', None) == 'True':
     # Run doxygen to produce the XML consumed by Breathe. The Doxyfile's
     # clang-assisted parsing options hard-code host-specific include paths
     # that don't exist on Read the Docs, so turn that off and append the
     # overrides to the Doxyfile (later settings win).
     root = pathlib.Path(__file__).resolve().parent.parent
     overrides = '\n'.join([
          'CLANG_ASSISTED_PARSING = NO',
          'GENERATE_HTML = NO',
          'GENERATE_XML = YES',
     ])
     doxyfile = (root / 'Doxyfile').read_text() + '\n' + overrides + '\n'
     subprocess.run(['doxygen', '-'], cwd=root, input=doxyfile, text=True, check=True)
