# Changer

Changer is linux-based anti-forensics tool for reading, extracting, modifying and destroying selected information from files, partitions and whole disks using gained byte offsets from commands like grep. Basically, it's srm but on manual scale with surgical precision and bunch of other things.

## Dev status: `buggy, but rollin'`

## Installation

I was too lazy to make package. You must gcc your way into usage:
- `gcc changer.c -o changer`

P.S. My compailer spec: 

    Using built-in specs.
    COLLECT_GCC=gcc
    COLLECT_LTO_WRAPPER=/usr/lib/gcc/x86_64-pc-linux-gnu/9.2.0/lto-wrapper
    Target: x86_64-pc-linux-gnu
    Configured with: ....
    Thread model: posix
    gcc version 9.2.0 (GCC) 

## Options:

    Usage: changer [OPTION...] MEM_OFFSET_LIST (Format: "INT INT INT INT...")
    changer -- A linux-based anti-forensics tool for reading, extracting, modifying
    and destroying data from files.

    -b, --buffer-size=INT      Define read/write buffer size. Default value: 42.
    -d, --destroy-data         Destroy data indexed with offsets and buffer size.
                                Data will be overwritten with Lorem Ipsum.
    -i, --infile=FILE_PATH     Define input file path or disk image.
    -l, --destroy-with-lorem   Use this command in combination with '-d' flag to
                                finish destroying data with Lorem Ipsum
                                overwrite.
    -m, --modify-data=STRING   Define string to be written in place of data
                                indexed with offsets and buffer size. Buffer size
                                will be set to string length.
    -r, --read-data            Read data indexed with offsets and buffer size and
                                print it.
    -s, --save-data=FILE_PATH  Save data indexed with offsets and buffer size in
                                defined file. Default file path:
                                './saved_data.txt'.
    -v, --verbose              Produce verbose output.
    -?, --help                 Give this help list
        --usage                Give a short usage message

    Mandatory or optional arguments to long options are also mandatory or optional
    for any corresponding short options.

    From Lazar Markovic.

## Small file example

We will use file called `test` with following content:

    Danas je 111savrsen dan 111i sve je 111lepo, a 111napolju je 1111hladno.
    I 111tako dalje.

We will conceive following command:

    grep -b -o -a '111' test | sed 's/:.*/ /g' | xargs | ./changer -i test -b 7 -d -l

WTF.. is this?

Lets see progression of piping, lets break down the command:
1. `grep` command searches for all occurrences of fragment '111' in given file `test` and returns list of byte offsets in relation to the beggining of file:

        9:111
        24:111
        36:111
        47:111
        61:111
        75:111
2. good old `sed` transforms output in following way:
        
        9 
        24 
        36 
        47 
        61 
        75
3. `xargs`, you know:
       
        9 24 36 47 61 75

4. `changer`, finnaly, takes offsets from xargs, takes input file `test`, we set buffer size `-b` to 7 (number of characters to be destroyed starting from offfset position) and use `-d` command to destroy - to overwrite data [35 times with gibberish](https://en.wikipedia.org/wiki/Gutmann_method) and then '-l' to overwrite with dumb Lorem Ipsum (yes, it's stupid idea, but random numbers/values are no better -> machine learning is needed here -> there's work to be done here in future, to mask usage of program itself). And resulting file is:

        Danas je Lorem isen dan Lorem ie je Lorem i, a Lorem ilju je Lorem idno.
        I Lorem i dalje. 

## Whole partition example
Lets say that you know (or suspect) that somewhere on your HDD or SSH there is forgotten and lost sensitive data, and that you want to get rid of it, but not destroy the disk or wipe it using existing tools. This is how to do so:
1. First of try to remember some part of it, and run grep and sed like this:
        
        grep -b -i -a 'sensitive data fragment' /dev/sdX | sed 's/:.*/ /g' > out.txt

2. Command above might take a while to complete, depends on HDD/SSD rw speed and partition size. Now lets import data to the changer and review occurrences in order to find buffer size (repeat command untill we see all of sensitive data in frame between '->' and '<-' outputs - works only in verbrose `-v` mode):
    
        cat out.txt | xargs | ./changer -i test -v -b some_buffer_size -r

3. And finally when we come up with buffer size - we destroy every occurrence of sensitive data on disk:

        cat out.txt | xargs | ./changer -i test -v -b final_buffer_size -d

4. Confirm operation (to some degree), run:

        grep -a -B 1 -A 2 'sensitive data fragment' /dev/sdaX

    If nothing is printed, you're fine.

## Little anti-forensic manual and some notes
If one wants to conceal execution of those commands (on Liux).
One needs to temporarily disable system logging and bash/zsh command history logging.
On Arch Linux one needs to:

- Temporarily disable physical logging:
  1. Go to `/etc/systemd/journald.conf` , uncomment `Storage=auto` and set to `Storage=none`
  2. Restart journald service with: 
     - `sudo systemctl restart systemd-journald.service`

- Disable zsh command history logging (not bash):
  1. Run command `setopt histignorespace`
  2. Use space before command and it will never be logged into `zsh.history` file

- Better solution will be to run all those programs from Live OS, to even hide the fact that you had this peace of software on your machine. 
- And the best solution will be to [melt your HDD/SSH in manetic flux](https://www.backyardscient.ist/induction-heater).

In case one has sensitive information already stored somewhere but not deleted, use [Van Hauser of THC's](https://github.com/vanhauser-thc) secure-delete package with command:
    - `sudo srm -vzr file_name_or_folder`

## Refs:
  - [Takkat on askubuntu.com](https://askubuntu.com/a/57580)
  - [flamsmark on askubuntu.com](https://askubuntu.com/a/58420)
  - [Secure Erace](http://cseweb.ucsd.edu/~m3wei/assets/pdf/FMS-2010-Secure-Erase.pdf)
  - [Reliably Erasing Data From Flash-Based Solid State Drives](https://www.usenix.org/legacy/events/fast11/tech/full_papers/Wei.pdf)
  - [SoK: Secure Data Deletion](http://pages.cpsc.ucalgary.ca/~joel.reardon/reardon-ieee2013.pdf)
  - [On multiple overwritting passes](https://security.stackexchange.com/questions/10464/why-is-writing-zeros-or-random-data-over-a-hard-drive-multiple-times-better-th)
  - [Secure Deletion of Data from Magnetic and Solid-State Memory](https://www.cs.auckland.ac.nz/~pgut001/pubs/secure_del.html)
  - [Data Remanence in Flash Memory Devices](https://www.cl.cam.ac.uk/~sps32/DataRem_CHES2005.pdf)
  - [Gutmann method](https://en.wikipedia.org/wiki/Gutmann_method)
  - [srm manual](http://srm.sourceforge.net/srm.html)
  - [NIST 800-88 - Guidelines for Media Sanitization](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-88r1.pdf)
  - [DoD 5220.22-M Wiping Standard](https://www.blancco.com/blog-dod-5220-22-m-wiping-standard-method/)
  - [Mersenne Twister](https://en.wikipedia.org/wiki/Mersenne_Twister) 
  - [Mersenne Twister in C by Takuji Nishimura](http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/VERSIONS/C-LANG/deifik.c)