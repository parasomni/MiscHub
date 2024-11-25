#!/bin/python3
# automated backup tool

import argparse 
import time
import os
import shutil

from tqdm import tqdm

print_path = '/tmp/print.tmp'
list_path = '/tmp/list.tmp'

def run_backup(src, dst, time_intvall):
    while True:
        print("preparing backup...")
        time_sec = int(time_intvall) * 60 * 60
        analyze_directories(src, dst)
        print("\r\nbackup done")
        print(f"next backup in {time_intvall}h")
        time.sleep(time_sec)
        
def scan_dst(dst):
    for path, dirs, files in os.walk(dst):
        for file in files:
            path = path.replace(dst, '')
            file_path = path + '/' + file
            with open(list_path, 'a') as f:
                f.write(file_path + '\r\n')
            f.close()  

def scan_src(src, dst):
    dst_list = ''
    for path, dirs, files in os.walk(src):
        for file in files:
            backup_path = path.replace(src, '')
            dst_path = dst + backup_path
            src_file_path = path + '/' + file
            backup_file = backup_path + '/' + file
            dst_file_path = dst + backup_file
            print_file = backup_path + '/' + file
            check_dir(dst_path)
            try: 
                with open(list_path, 'r') as f:
                    dst_list = f.read()
                f.close()
                if backup_file not in dst_list:
                    with open(print_path, 'a') as f:
                        f.write(print_file + '\r\n')
                    f.close()
                else: 
                    src_file_size = os.path.getsize(src_file_path)
                    dst_file_size = os.path.getsize(dst_file_path)  
                    if src_file_size == dst_file_size:
                        pass
                    else:
                        with open(print_path, 'a') as f:
                            f.write(print_file + '\r\n')
                        f.close()                
            except FileNotFoundError:
                with open(print_path, 'a') as f:
                    f.write(print_file + '\r\n')
                f.close()                
        
def analyze_directories(src, dst):
    print('analyzing directories...')
    scan_dst(dst)
    scan_src(src, dst)   
    print('analyzing directories done')
    copy_files(src, dst)
                    
                
def copy_files(backup_path, blackbox):
    try:
        file_count = len(list(open(print_path, "rb")))
        print('files to copy: ', file_count)
        with open(print_path, 'rb') as wordlist:
            for file in tqdm(wordlist, total=file_count, unit='file'):
                try:
                    src_file_path = backup_path + file.decode().strip()
                    dst_file_path = blackbox + file.decode().strip()                    
                    shutil.copy(src_file_path, dst_file_path)
                except:
                    pass
    except FileNotFoundError:
        print('no backup necessary')
        
    print('cleaning up...')
    try:
        os.remove(print_path)
        print('removed ', print_path)
        os.remove(list_path)
        print('removed ', list_path)
    except:
        pass

def check_dir(dirPath):
    if os.path.exists(str(dirPath)):
        pass
    else:
        print(f'directory {dirPath} not exists --> creating...')
        os.makedirs(dirPath)

def main():
    parser = argparse.ArgumentParser(description="")
    parser.add_argument("-src", dest="SOURCE", help="directory to backup automated")
    parser.add_argument("-dst", dest="DESTINATION", help="directory to backup source")
    parser.add_argument("-t", dest="TIME", help="time intervall to backup in hours")
    args = parser.parse_args()
    run_backup(args.SOURCE, args.DESTINATION, args.TIME)


main()
    
    