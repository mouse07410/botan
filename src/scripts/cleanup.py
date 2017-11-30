#!/usr/bin/env python

"""
Implements the "make clean" target

(C) 2017 Jack Lloyd

Botan is released under the Simplified BSD License (see license.txt)
"""

import os
import sys
import re
import optparse  # pylint: disable=deprecated-module
import logging
import json
import shutil

def remove_dir(d):
    try:
        if os.access(d, os.X_OK):
            logging.debug('Removing directory "%s"', d)
            shutil.rmtree(d)
        else:
            logging.debug('Directory %s was missing', d)
    except Exception as e: # pylint: disable=broad-except
        logging.error('Failed removing directory "%s": %s', d, e)

def remove_file(f):
    try:
        if os.access(f, os.R_OK):
            logging.debug('Removing file "%s"', f)
            os.unlink(f)
        else:
            logging.debug('File %s was missing', f)
    except Exception as e: # pylint: disable=broad-except
        logging.error('Failed removing file "%s": %s', f, e)

def remove_all_in_dir(d):
    if os.access(d, os.X_OK):
        logging.debug('Removing all files in directory "%s"', d)

        for f in os.listdir(d):
            remove_file(os.path.join(d, f))

def main(args=None):
    if args is None:
        args = sys.argv

    parser = optparse.OptionParser()
    parser.add_option('--build-dir', default='build', metavar='DIR',
                      help='specify build dir to clean (default %default)')

    parser.add_option('--distclean', action='store_true', default=False,
                      help='clean everything')
    parser.add_option('--verbose', action='store_true', default=False,
                      help='noisy logging')

    (options, args) = parser.parse_args(args)

    logging.basicConfig(stream=sys.stderr,
                        format='%(levelname) 7s: %(message)s',
                        level=logging.DEBUG if options.verbose else logging.INFO)

    if len(args) > 1:
        logging.error("Unknown arguments")
        return 1

    build_dir = options.build_dir

    if os.access(build_dir, os.X_OK) != True:
        logging.error("Unable to access build directory")
        return 1

    build_config_path = os.path.join(build_dir, 'build_config.json')
    build_config_str = None

    try:
        build_config_file = open(build_config_path)
        build_config_str = build_config_file.read()
        build_config_file.close()
    except Exception: # pylint: disable=broad-except
        # Ugh have to do generic catch as different exception type thrown in Python2
        logging.error("Unable to access build_config.json in build dir")
        return 1

    build_config = json.loads(build_config_str)

    #print(json.dumps(build_config, sort_keys=True, indent=3))

    if options.distclean:
        build_dir = build_config['build_dir']
        remove_file(build_config['makefile_path'])
        remove_dir(build_dir)
    else:
        for dir_type in ['libobj_dir', 'cliobj_dir', 'testobj_dir']:
            dir_path = build_config[dir_type]
            remove_all_in_dir(dir_path)

    remove_file(build_config['cli_exe'])
    remove_file(build_config['test_exe'])

    matches_libname = re.compile('^' + build_config['lib_basename'] + '.([a-z]+)')

    known_suffix = ['a', 'so', 'dll', 'manifest', 'exp']

    for f in os.listdir(build_config['out_dir']):
        match = matches_libname.match(f)
        if match and match.group(1) in known_suffix:
            remove_file(os.path.join(build_config['out_dir'], f))

    if options.distclean:
        if 'generated_files' in build_config:
            for f in build_config['generated_files'].split(' '):
                remove_file(f)

if __name__ == '__main__':
    sys.exit(main())
