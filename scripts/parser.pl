
#! /usr/bin/perl
use strict;
 
use mrw::Targets; 
use mrw::Inventory;
use Getopt::Long; # For parsing command line arguments
use YAML::XS 'LoadFile'; # For loading and reading of YAML file
use Data::Dumper;

my @listofFru; #Array of hash

# Globals
my $force           = 0;
my $serverwizFile  = "";
my $debug           = 0;
my $outputFile     = "";
my $verbose         = 0;

# Command line argument parsing
GetOptions(
"f"   => \$force,             # numeric
"i=s" => \$serverwizFile,    # string
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

my @inventory = Inventory::getInventory($targetObj);
for my $item (@inventory) {
    my $isFru = 0, my $fruID = 0, my $fruType = "";
          #And this gets a FRU ID
    if (!$targetObj->isBadAttribute($item->{TARGET}, "FRU_ID")) {
        $fruID = $targetObj->getAttribute($item->{TARGET}, "FRU_ID");
        $isFru = 1;
    }

    if (!$targetObj->isBadAttribute($item->{TARGET}, "TYPE")) {
        $fruType = $targetObj->getAttribute($item->{TARGET}, "TYPE");
    }
    push @listofFru, { FRUID => $fruID,FRUType => $fruType, ObjectPath => $item->{OBMC_NAME} };
    

=begin
    if ($isFru) {
        my $conns = $targetObj->findConnections($item, "LOGICAL_ASSOCIATION");
        if ($conns ne "") {
            print "Connections Found \n";
            # This conn is associated with a FRU
            for my $conn (@{$conns->{CONN}}) {
                my $source = $conn->{SOURCE};
                my $nonFruDeviceName = $source->{OBMC_NAME};
                #there I can put a check whether this is interested device or not.
                if (!$targetObj->isBadAttribute($source->{TARGET}, "TYPE")) {
                    $fruType = $targetObj->getAttribute($source->{TARGET}, "TYPE");
                    print "FRU TYPE: $fruType\n";
                }
            }                

        }
    }
=cut
}

printDebug("\n======================================================================\n");
printDebug("\nInventory FRU\n");
my $i = 0;

for $i ( 0 .. $#listofFru ) {
    printDebug("\n");
    for my $key ( sort keys %{ $listofFru[$i] }) {
        my $value = $listofFru[$i]{$key};
        printDebug( "$key :: $value, ");

    }
}
printDebug("\n======================================================================\n");

generateMRWYamlFile();
generateInventoryYAMLFile();
#------------------------------------END OF MAIN-----------------------

#Generate the yaml file
sub generateMRWYamlFile
{
    my $fileName = $outputFile;
    my $groupCopy = '';
    my $ledCopy = '';
    open(my $fh, '>', $fileName) or die "Could not open file '$fileName' $!";

    for $i ( 0 .. $#listofFru ) {
        print $fh "\n";
        for my $key ( sort keys %{ $listofFru[$i] }) {
            my $value = $listofFru[$i]{$key};
            if ( $key eq "FRUID") {
                print $fh "- FRUID: ".$value;
                print $fh "\n";
            } else {
                print $fh "  $key: ".$value;
                print $fh "\n";
            }
            
        }
    }
    close $fh;
}

sub generateInventoryYAMLFile
{
    my $fileName = 'inventory-gen.yaml';
    open(my $fh, '>', $fileName) or die "Could not open file '$fileName' $!";
    my $mrwConfig = LoadFile($outputFile);
    my $fruTypeConfig = LoadFile('fru_types.yaml');

    foreach my $fru (@{$mrwConfig}) {
        print $fh "- FRUID: ".$fru->{FRUID};
        print $fh "\n";
        print $fh "  Instances:";
        print $fh "\n";
        next if ( $fru->{FRUType} eq 'NA' );
        #write the meta data for this FRU
        writeMetaData($fru->{FRUType},$fru->{ObjectPath},$fruTypeConfig,$fh);
        #write the meta data for the associated devices
        my $devices = $fru->{LogicalDevices};
        foreach my $device (@{$devices}) {
            writeMetaData($device->{FRUType},$device->{ObjectPath},$fruTypeConfig,$fh);
        }
    }
    close $fh;
                                
}

sub writeMetaData
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
            #Walk over all the interfces as it needs to be written
            foreach my $interface (@{$interfaces}) {
                print $fh "        - Interface: ".$interface->{Interface};
                print $fh "\n";
                print $fh "          Properties: ";
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
    $0 -i [XML filename] -o [Output filename] [OPTIONS]
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
