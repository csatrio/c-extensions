#!/usr/bin/env python3
# encoding: utf-8

from distutils.core import setup, Extension

uv_module = Extension('uvmodule',
              include_dirs=['/usr/include','/usr/local/include'],
              libraries=['pthread','uv','http_parser'],
              library_dirs=['/usr/lib','/usr/local/lib'],
              sources = ['uvmodule.c']
        )

setup(name='uvmodule',
      version='0.1.0',
      description='libuv module written in C',
      ext_modules=[uv_module])