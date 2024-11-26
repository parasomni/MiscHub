#!/usr/bin/python3
import os

def check_dir(dirPath):
    if os.path.exists(str(dirPath)):
        print(f'Directory {dirPath} exists.')
        pass
    else:
        print(f'Directory {dirPath} not exists --> creating...')
        os.makedirs(dirPath)

def make_dirs():
    print("running prescan for directories...")
    ultron_hashes = '/etc/ultron/ultron-hashes/'
    ultron_logs = '/etc/ultron/ultron-logs'
    ultron_dir_logs = '/etc/ultron//ultron-dir-logs'
    ultron_main_logs = '/etc/ultron/main-logs'
    check_dir(ultron_dir_logs)
    check_dir(ultron_hashes)
    check_dir(ultron_logs)
    check_dir(ultron_main_logs)
    print("setting configuration")
    os.system("mv ultron.config /etc/ultron/ultron.config")
    print("installing triggers")
    os.system("mv ultron.py /usr/bin/ultron")


if __name__ == "__main__":
    make_dirs()
