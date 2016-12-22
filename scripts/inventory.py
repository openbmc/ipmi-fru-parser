#!/usr/bin/env python

import os
import yaml
import argparse
from mako.template import Template


def generate_hpp(inventory_yaml, output_dir):
    with open(inventory_yaml, 'r') as f:
        ifile = yaml.safe_load(f)

        # Render the mako template
        script_dir = os.path.dirname(os.path.realpath(__file__))

        t = Template(filename=os.path.join
            (script_dir,
             "writefru.mako.hpp"))

        output_hpp = os.path.join(output_dir, "fru-gen.hpp")
        with open(output_hpp, 'w') as fd:
            fd.write(t.render(fruDict=ifile))


def main():

    valid_commands = {
        'generate-hpp': generate_hpp
    }
    parser = argparse.ArgumentParser(
        description="IPMI FRU parser and code generator")

    parser.add_argument(
        '-i', '--inventory_yaml', dest='inventory_yaml',
        default='example.yaml', help='input inventory yaml file to parse')

    parser.add_argument(
        "-o", "--output-dir", dest="outputdir",
        default=".",
        help="output directory")

    parser.add_argument(
        'command', metavar='COMMAND', type=str,
        choices=valid_commands.keys(),
        help='Command to run.')

    args = parser.parse_args()

    if (not (os.path.isfile(args.inventory_yaml))):
        print "Can not find input yaml file " + args.inventory_yaml
        exit(1)

    function = valid_commands[args.command]
    function(args.inventory_yaml, args.outputdir)

if __name__ == '__main__':
    main()
