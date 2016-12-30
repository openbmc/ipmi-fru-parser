#! /usr/bin/perl
use strict;
use warnings;

use mrw::Targets;
use mrw::Inventory;
use Getopt::Long; # For parsing command line arguments
use YAML::XS 'LoadFile'; # For loading and reading of YAML file
use Data::Dumper;

# Globals
my $force           = 0;
my $serverwizFile  = "";
my $debug           = 0;
my $outputFile     = "";
my $metaDataFile   = "";
my $verbose         = 0;

# Command line argument parsing
GetOptions(
"f"   => \$force,            # numeric
"i=s" => \$serverwizFile,    # string
"m=s" => \$metaDataFile,     #string
"o=s" => \$outputFile,       # string
"d"   => \$debug,
"v"   => \$verbose,
)
or printUsage();

if (($serverwizFile eq "") or ($outputFile eq ""))
{
    printUsage();
}

my $targetObj = Targets->new;
$targetObj->loadXML($serverwizFile);

#open the mrw xml and the metaData file for the fru.
#Fetch the FRU id,type,object path from the mrw.
#Get the metadata for that fru from the metadata file.
#Merge the data into the outputfile.

open(my $fh, '>', $outputFile) or die "Could not open file '$outputFile' $!";
my $fruTypeConfig = LoadFile($metaDataFile);

my @inventory = Inventory::getInventory($targetObj);
for my $item (@inventory) {
    my $isFru = 0, my $fruID = 0, my $fruType = "";
    #Interested in FRU_ID and TYPE
    if (!$targetObj->isBadAttribute($item->{TARGET}, "FRU_ID")) {
        $fruID = $targetObj->getAttribute($item->{TARGET}, "FRU_ID");
        $isFru = 1;
    }

    if (!$targetObj->isBadAttribute($item->{TARGET}, "TYPE")) {
        $fruType = $targetObj->getAttribute($item->{TARGET}, "TYPE");
    }

    printDebug ("FRUID => $fruID,FRUType => $fruType, ObjectPath => $item->{OBMC_NAME}");
    #Don't write the fru details in to the file
    #whose type is NA
    next if ( $fruType eq 'NA');
    print $fh "- FRUID: ".$fruID;
    print $fh "\n";
    print $fh "  Instances:";
    print $fh "\n";

    writeToFile($fruType,$item->{OBMC_NAME},$fruTypeConfig,$fh);
}
close $fh;

#------------------------------------END OF MAIN-----------------------

#Get the metdata for the incoming frutype from the loaded config file.
#Write the FRU data into the output file

sub writeToFile
{
    my $fruType = $_[0];#fru type
    my $instancePath = $_[1];#instance Path
    my $fruTypeConfig = $_[2];#loaded config file (frutypes)
    my $fh = $_[3];#file Handle

    #walk over all the fru types and match for the incoming type
    foreach my $type (@{$fruTypeConfig}) {
        if  ($type->{FRUType} eq $fruType) {
            print $fh "    - Instance: ".$instancePath;
            print $fh "\n";
            print $fh "      Interfaces:";
            print $fh "\n";
            my $interfaces = $type->{Interfaces};
            #Walk over all the interfaces as it needs to be written
            foreach my $interface (@{$interfaces}) {
                print $fh "        - Interface: ".$interface->{Interface};
                print $fh "\n";
                print $fh "          Properties:";
                print $fh "\n";
                #walk over all the properties as it needs to be written
                foreach my $property (@{$interface->{Properties}}) {
                    #will write property named "Property" first then
                    #other properties.
                    print $fh "             - Property: ".$property->{Property};
                    print $fh "\n";
                    for my $key (sort keys %{$property}) {
                        if ($key ne 'Property') {
                            print $fh "               $key: ".$property->{$key};
                            print $fh "\n";
                        }
                    }
                }
            }
        }
    }

}

# Usage
sub printUsage
{
    print "
    $0 -i [MRW filename] -m [MetaData filename]-o [Output filename] [OPTIONS]
Options:
    -f = force output file creation even when errors
    -d = debug mode
    -v = verbose mode - for verbose o/p from Targets.pm
        \n";
    exit(1);
}

# Helper function to put debug statements.
sub printDebug
{
    my $str = shift;
    print "DEBUG: ", $str, "\n" if $debug;
}

