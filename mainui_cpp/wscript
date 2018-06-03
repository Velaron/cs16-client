#! /usr/bin/env python
# encoding: utf-8
# a1batross, mittorn, 2018

from waflib import Logs
import os

top = '.'

def options(opt):
	opt.add_option('--use-stb', action = 'store_true', dest = 'USE_STBTT',
		help = 'prefer stbtt over freetype')

	return

def configure(conf):
	if conf.options.DEDICATED:
		return

	conf.env.USE_STBTT = conf.options.USE_STBTT
	
	if conf.env.DEST_OS == 'darwin':
		conf.env.USE_STBTT = True
	
	if not conf.env.USE_STBTT and conf.env.DEST_OS != 'win32':
		errormsg = '{0} not available! Install {0} development package. Also you may need to set PKG_CONFIG_PATH environment variable'
		
		try:
			conf.check_cfg(package='freetype2')
		except conf.errors.ConfigurationError:
			conf.fatal(errormsg.format('freetype2'))
		try:
			conf.check_cfg(package='fontconfig')
		except conf.errors.ConfigurationError:
			conf.fatal(errormsg.format('fontconfig'))

def get_subproject_name(ctx):
	return os.path.basename(os.path.realpath(str(ctx.path)))

def build(bld):
	bld.load_envs()
	bld.env = bld.all_envs[get_subproject_name(bld)]

	if bld.env.DEDICATED:
		return

	libs = []

	# basic build: dedicated only, no dependencies
	if bld.env.DEST_OS != 'win32' and not bld.env.USE_STBTT:
		libs += ['FREETYPE2', 'FONTCONFIG']

	source = bld.path.ant_glob([
		'*.cpp', 
		'font/*.cpp', 
		'menus/*.cpp', 
		'menus/dynamic/*.cpp', 
		'model/*.cpp',
		'controls/*.cpp'])

	includes = ['.', 'utl/', 'font/', 'controls/', 'menus/', 'model/', '../common', '../engine', '../pm_shared']

	bld.shlib(
		source   = source,
		target   = 'menu',
		features = 'cxx',
		includes = includes,
		use      = libs)
