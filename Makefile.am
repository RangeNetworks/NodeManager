#
# Copyright 2013, 2014 Range Networks, Inc.
#
# This software is distributed under the terms of the GNU Public License.
# See the COPYING file in the main directory for details.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

include $(top_srcdir)/Makefile.common

AM_CPPFLAGS = $(STD_DEFINES_AND_INCLUDES)
AM_CXXFLAGS = -Wall -O3

noinst_LTLIBRARIES = libnodemanager.la

libnodemanager_la_SOURCES = \
	NodeManager.cpp \
	JsonBox-0.4.3/src/Array.cpp \
	JsonBox-0.4.3/src/Convert.cpp \
	JsonBox-0.4.3/src/Escaper.cpp \
	JsonBox-0.4.3/src/IndentCanceller.cpp \
	JsonBox-0.4.3/src/Indenter.cpp \
	JsonBox-0.4.3/src/Object.cpp \
	JsonBox-0.4.3/src/SolidusEscaper.cpp \
	JsonBox-0.4.3/src/Value.cpp

noinst_HEADERS = \
	NodeManager.h

