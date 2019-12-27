# Changer

Changer is linux-based (anti-)forensics tool for reading, extracting, modifying and securely deleting precisely selected either indexed or deleted data from files or whole disks using. Basically, it's srm (secure_delete) but on manuallly defined scale with surgical precision and bunch of other options.

## Dev status: `buggy, but rollin'`

## Intro into subject
Privacy is one of biggest issues in modern world. Deleted data is not deleted, it is hiding somewhere, waiting to be recovered by someone seeking it.

No matter if you're just a privacy freak like me, or desperate gov whistleblower. You will want to cover your tracks. Any documents that you deleted via regular `Trash Bin` is and will be, easily recoverable.

Even so called "[Data sanitization guidelines](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-88r1.pdf)" [fail to make data unrecoverable](https://www.cl.cam.ac.uk/~sps32/DataRem_CHES2005.pdf), if attacker one has enough time and some really expensive tools at his disposal.

So, why not at least make it harder and more expensive for attacker to recover data?

This software enables it's user, after he had located either indexed or deleted data on disk using tools like `grep`, to recover it, extract it, modify it or destroy it - make it [almost](https://www.cl.cam.ac.uk/~sps32/DataRem_CHES2005.pdf) unrecoverable.

It might be [overkill for SSDs](https://security.stackexchange.com/questions/10464/why-is-writing-zeros-or-random-data-over-a-hard-drive-multiple-times-better-th) but in this project I use [Gutmann method](https://en.wikipedia.org/wiki/Gutmann_method) for making data unrecoverable. It includes 35 steps - different overwrites of specified data. And internal scrambling of steps using [pRNG Mersenne Twister](https://en.wikipedia.org/wiki/Mersenne_Twister). And then, speaking of overkills, optionally fills it with infinitely recurring Lorem Ipsum string.

Problem of reallocated sectors by disk itself, which is big issue in this field, is not addressed here.

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

## Options - help output:

    Usage: changer [OPTION...] MEM_OFFSET_LIST (Format: "INT INT INT INT...")
    Changer is linux-based (anti-)forensics tool for reading, extracting, modifying
    and securely deleting precisely selected either indexed or deleted data from
    files or whole disks using. Basically, it's srm (secure_delete) but on
    manuallly defined scale with surgical precision and bunch of other options.

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
Lets say that you know (or suspect) that somewhere on your HDD or SSH there is forgotten and lost sensitive data, and that you want to get rid of it (make it unrecoverable), but not destroy the disk itself or wipe it clean using existing tools. This is how to do so:
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


In case one has sensitive information already stored somewhere but not deleted, use [Van Hauser of THC's](https://github.com/vanhauser-thc) secure-delete package with command:
    - `sudo srm -vzr file_name_or_folder`

## Outro

In the end. No data is save on any medium - if it is in hands of and attacker.

Using some techniques of secure deletion data can get safer than not using any at all. Even with encryption, attacker might use 'advanced' Rubber-hose cryptanalysis called "[lead pipe](https://xkcd.com/538/)".

This software gives provides "good-enough" solution when user wants to wipe desired fragment of information from disk but doesn't want or can't wipe whole disk clean and physically destroy it.

Regarding problem of reallocated sectors i will [quote user from stackexchange.com:

> "In the end, it's a question of "what data" you want to "protect" by erasing it, and how important it is that the erased data will be unrecoverable in any potential case. Let's put it this way: if you want to delete your personal dairy, you probably don't need to overwrite each and every free sector... but if you're working on plans for a nuclear power plant or some "secret project" for your government, you'll not want to let a single byte as is." 
> ~user6373 on stackexchange.com

Anyhow best "secure deletion" of data is to [melt your HDD/SSH in manetic flux](https://www.backyardscient.ist/induction-heater).

## Refs:
  - [Takkat on askubuntu.com](https://askubuntu.com/a/57580)
  - [flamsmark on askubuntu.com](https://askubuntu.com/a/58420)
  - [Secure Erace](http://cseweb.ucsd.edu/~m3wei/assets/pdf/FMS-2010-Secure-Erase.pdf)
  - [Reliably Erasing Data From Flash-Based Solid State Drives](https://www.usenix.org/legacy/events/fast11/tech/full_papers/Wei.pdf)
  - [SoK: Secure Data Deletion](http://pages.cpsc.ucalgary.ca/~joel.reardon/reardon-ieee2013.pdf)
  - [On multiple overwritting passes](https://security.stackexchange.com/questions/10464/why-is-writing-zeros-or-random-data-over-a-hard-drive-multiple-times-better-th)
  - [Secure Deletion of Data from Magnetic and Solid-State Memory](https://www.cs.auckland.ac.nz/~pgut001/pubs/secure_del.html)
  - [Data Remanence in Flash Memory Devices](https://www.cl.cam.ac.uk/~sps32/DataRem_CHES2005.pdf)
  - [Overwriting Hard Drive Data: The Great Wiping Controversy](https://www.vidarholen.net/~vidar/overwriting_hard_drive_data.pdf)
  - [Gutmann method](https://en.wikipedia.org/wiki/Gutmann_method)
  - [srm manual](http://srm.sourceforge.net/srm.html)
  - [NIST 800-88 - Guidelines for Media Sanitization](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-88r1.pdf)
  - [DoD 5220.22-M Wiping Standard](https://www.blancco.com/blog-dod-5220-22-m-wiping-standard-method/)
  - [Mersenne Twister](https://en.wikipedia.org/wiki/Mersenne_Twister) 
  - [Mersenne Twister in C by Takuji Nishimura](http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/VERSIONS/C-LANG/deifik.c)


## License
Changer is [MIT licensed](https://en.wikipedia.org/wiki/MIT_License).