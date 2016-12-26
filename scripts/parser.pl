#! /usr/bin/perl
use strict;
use warnings;

use mrw::Targets;
use mrw::Inventory;
use Getopt::Long; # For parsing command line arguments
use Data::Dumper;

# Globals
my $serverwizFile  = "";
my $debug           = 0;

# Command line argument parsing
GetOptions(
"i=s" => \$serverwizFile,    # string
"d"   => \$debug,
)
or printUsage();

if ($serverwizFile eq "")
{
    printUsage();
}

my $targetObj = Targets->new;
$targetObj->loadXML($serverwizFile);

#Fetch the FRU id,type,object path from the mrw.

my @inventory = Inventory::getInventory($targetObj);
for my $item (@inventory) {
    my $fruID = 0, my $fruType = "";
    #Interested in FRU_ID and TYPE
    if (!$targetObj->isBadAttribute($item->{TARGET}, "FRU_ID")) {
        $fruID = $targetObj->getAttribute($item->{TARGET}, "FRU_ID");
    }

    if (!$targetObj->isBadAttribute($item->{TARGET}, "TYPE")) {
        $fruType = $targetObj->getAttribute($item->{TARGET}, "TYPE");
    }
    #Not Interested in the fru whose type is NA.
    next if ( $fruType eq 'NA');
    printDebug ("FRUID => $fruID,FRUType => $fruType, ObjectPath => $item->{OBMC_NAME}");
}

# Usage
sub printUsage
{
    print "
    $0 -i [MRW filename] [OPTIONS]
Options:
    -d = debug mode
        \n";
    exit(1);
}

# Helper function to put debug statements.
sub printDebug
{
    my $str = shift;
    print "DEBUG: ", $str, "\n" if $debug;
}
