import os
import subprocess
from pathlib import Path
import argparse
import logging

import yaml

logging.basicConfig()
logger = logging.getLogger('check-all')
logger.setLevel(logging.INFO)


def parse_args():
    parser = argparse.ArgumentParser(
        description='clang_format_all starts at this directory and drills down recursively')

    parser.add_argument('--config', type=str, required=True,
                        help='Yaml file path with configuration.')

    args = parser.parse_args()
    return args


def check_all_walk_recursive(clang_format_command: str, root_dir: str, exclude_files=None, file_extensions=None):
    if exclude_files is None:
        exclude_files = set()
    if file_extensions is None:
        file_extensions = []

    for root, dirs, files in os.walk(root_dir):
        if dirs:
            for d in dirs:
                check_all_walk_recursive(clang_format_command, (os.path.join(root, d)), exclude_files, file_extensions)
        for file in files:
            path = Path(os.path.join(root, file))
            if str(path) in exclude_files:
                continue
            if path.suffix in file_extensions:
                if subprocess.run([clang_format_command, "--dry-run", "--Werror", "-style=file",
                                   path]).returncode != 0:
                    logger.info("\"%s\": does not comply to format according to clang-format", path)
                    exit(-1)
                else:
                    logger.info("\"%s\": looks fine according to clang-format", path)


def format_all_walk_recursive(clang_format_command: str, root_dir: str, exclude_files=None, file_extensions=None):
    if exclude_files is None:
        exclude_files = set()
    if file_extensions is None:
        file_extensions = []
    for root, dirs, files in os.walk(root_dir):
        if dirs:
            for d in dirs:
                format_all_walk_recursive(clang_format_command, (os.path.join(root, d)), file_extensions)
        for file in files:
            path = Path(os.path.join(root, file))
            if str(path) in exclude_files:
                continue
            if path.suffix in file_extensions:
                if subprocess.run([clang_format_command, "-style=file", "-i",
                                   path], capture_output=True).returncode != 0:
                    logger.info("\"%s\": An error occurred while parsing this file.", path)
                    exit(-1)
                else:
                    logger.info("\"%s\": parsed successfully.", path)


def get_all_files(root_dir: str) -> []:
    files_array = []
    for root, dirs, files in os.walk(root_dir):
        if dirs:
            for d in dirs:
                files_array += get_all_files((os.path.join(root, d)))
        for file in files:
            files_array.append(os.path.join(root, file))
    return files_array


def get_resolved_paths(unresolved_paths: [str], root_dir: str) -> [str]:
    resolved_paths = []
    for p in unresolved_paths:
        resolved_paths += get_all_files(str(Path(root_dir, p).resolve()))

    return set(resolved_paths)


def main():
    args = parse_args()
    excluded_dirs = None
    with open(args.config) as yaml_stream:
        config = yaml.load(yaml_stream, Loader=yaml.CSafeLoader)
        if 'exclude_dirs' not in config:
            logger.error(f"No key 'exclude_dirs' found in {args.config}")
            return -1
        if 'root_dir' not in config:
            logger.error(f"No key 'root_dir' found in {args.config}")
            return -1
        if 'check_all' not in config:
            logger.error(f"No key 'check_all' found in {args.config}")
            return -1
        if 'file_extensions' not in config:
            logger.error(f"No key 'file_extensions' found in {args.config}")
            return -1

        excluded_dirs = get_resolved_paths(config['exclude_dirs'], config["root_dir"])

    if config['check_all'] is True:
        check_all_walk_recursive(config["clang_format_command"], str(Path(config["root_dir"]).resolve()), excluded_dirs, config['file_extensions'])
    else:
        format_all_walk_recursive(config["clang_format_command"], str(Path(config["root_dir"]).resolve()), excluded_dirs, config['file_extensions'])

main()