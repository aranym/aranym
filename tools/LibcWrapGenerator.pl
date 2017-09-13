#!/usr/bin/env perl
#
# Copyright (C) 2011 Jan Niklas Hasse <jhasse@gmail.com>
# Copyright (C) 2013 Upstairs Laboratories Inc.
# Copyright (C) 2017 Thorsten Otto
#
# This library is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 2.1 of
# the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public 
# License along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# Authors:
#   Jan Niklas Hasse <jhasse@gmail.com>
#   Tristan Van Berkom <tristan@upstairslabs.com>
#   Thorsten Otto <admin@tho-otto.de>
#

require 5.008;
use strict;
use warnings;
use Getopt::Long;
use Pod::Usage;
use Config;
use Data::Dumper;

my $me = 'LibcWrapGenerator.pl';
my $VERSION = "1.0";

my $DEFAULT_TARGET = "2.11";

my %args = (
	'libdir' => undef,
	'output' => "libcwrap.h",
	'target' => $DEFAULT_TARGET,
	
	'man' => 0,
	'help' => 0,
	'version' => 0,
);


package MyParser;
use vars qw(@ISA @EXPORT %ESCAPES $VERSION);
@ISA = qw(Pod::Usage);

sub cmd_x {
	my ($self, $attrs, $text) = @_;
	if ($text eq 'me') {
		return $me;
	}
	if ($text eq 'default_target') {
		return $DEFAULT_TARGET;
	}
	if ($text eq 'output') {
		return $args{'output'};
	}
	return '';
}


package main;
	
# All symbols, containing the newest possible version before 'minimumVersion' if possible
my %symbolMap;
# All symbols which did not have any version > minimumVersion
my %filterMap;

sub print_version_and_exit()
{
	print "$me version $VERSION\n";
	print "running on Perl version " . join(".", map { $_||=0; $_*1 } ($] =~ /(\d)\.(\d{3})(\d{3})?/ )) . "\n";
	exit 0;
}

sub usage($)
{
	my ($exitval) = @_;
	# pod2usage(-exitstatus => 0, -verbose => 1, -output => '-') 
	my $fh;
	# open($fh, '|-', 'nroff -man' . ($exitval < 2 ? ' >&2' : '')) || die "$!";
	$fh = $exitval < 2 ? \*STDOUT : \*STDERR;
	my %opts = (
		'-input' => $0,
		'-output' => \*$fh,
		'-exitval' => $exitval,
		'-verbose' => 1,
	);
	my $parser = new MyParser(USAGE_OPTIONS => \%opts);
	if ($parser->can('select')) {
	    my $opt_re = '(?i)' .
	                     '(?:ARGUMENTS)' .
	                     '(?:\s*(?:AND|\/)\s*(?:ARGUMENTS))?';
	    $parser->select( '(?:SYNOPSIS|USAGE)\s*', $opt_re, "DESCRIPTION/$opt_re" );
	}
	$parser->parse_from_file($opts{'-input'}, $opts{'-output'});
	# close $fh;
	exit($opts{'-exitval'});
}


sub man()
{
	# pod2usage(-exitstatus => 0, -verbose => 2, -output => '-')
	my $fh;
	# open($fh, '|-', 'nroff -man | ' . ($Config{pager} || $ENV{PAGER} || '/bin/more' || '/bin/cat')) || die "$!";
	open($fh, '|-', ($Config{pager} || $ENV{PAGER} || '/bin/more' || '/bin/cat')) || die "$!";
	my %opts = (
		'-input' => $0,
		'-output' => \*$fh,
		'-exitval' => 0,
		'-verbose' => 2,
	);
	my $parser = new MyParser(USAGE_OPTIONS => \%opts);
	if ($parser->can('select')) {
	    $parser->select('.*');
	}
	$parser->parse_from_file($opts{'-input'}, $opts{'-output'});
	close $fh;
	exit($opts{'-exitval'});
}


sub usage_error($)
{
	my ($e) = @_;
	my $msg;
	$msg = $e;
	print STDERR "$msg\n";
	print STDERR "Run '$me --help' for a list of available command line options.\n";
	exit(2);
}


sub version_number($)
{
	my ($version) = @_;
	my @numbers = split('\.', $version);
	push @numbers, 0 if $#numbers < 1;
	push @numbers, 0 if $#numbers < 2;
	$version = ($numbers[0] * 1000 + $numbers[1]) * 1000 + $numbers[2];
	return $version;
}


sub parseLibrary($)
{
	my ($filename) = @_;
	my $fh;
	my $line;
	my $version;
	my $versionInMap;
	my $symbolName;
	my $minimumVersion = version_number($args{'target'});
	my $this_version;
	
	open($fh, "-|", "objdump -T $filename 2>/dev/null") || return;
	while (defined($line = <$fh>))
	{
		# strip CR and/or LF
		chop($line) if (substr($line, -1, 1) eq "\012");
		chop($line) if (substr($line, -1, 1) eq "\015");
		next if $line =~ qr(PRIVATE);
		next unless $line =~ qr{.*GLIBC_([0-9.]+)\)?[ ]*(.+)};
		$version = $1;
		$symbolName = $2;

		# Some versioning symbols exist in the objdump output, let's skip those
		next if $symbolName =~ /^GLIBC/;
		$this_version = version_number($version);
		if (!exists($symbolMap{$symbolName}))
		{
			$symbolMap{$symbolName} = $version;
			# First occurance of the symbol, if it's older
			# than the minimum required, put it in that table also
			if ($minimumVersion > $this_version)
			{
				$filterMap{$symbolName} = 1;
			}
		} else
		{
			my $versionInMap = version_number($symbolMap{$symbolName});
			# We want the newest possible version of a symbol which is older than the
			# minimum glibc version specified (or the only version of the symbol if
			# it's newer than the minimum version)
			if ($this_version <= $minimumVersion)
			{
				if ($versionInMap > $minimumVersion)
				{
					# What we have in the map is > minimumVersion, so we need this version
					$symbolMap{$symbolName} = $version;
				} elsif ($this_version > $versionInMap)
				{
					# What we have in the map is already <= minimumVersion, but we want
					# the most recent acceptable symbol
					$symbolMap{$symbolName} = $version;
				}
			} else
			{
				# If there are only versions > minimumVersion, then we want
				# the lowest possible version, this is because we try to provide
				# information in the linker warning about what version the symbol
				# was initially introduced in.
				if ($versionInMap > $minimumVersion && $versionInMap > $this_version)
				{
					$symbolMap{$symbolName} = $version;
				}
			}

			# While trucking along through the huge symbol list, remove symbols from
			# the 'safe to exclude' if there is a version found which is newer
			# than the minimum requirement
			if ($this_version > $minimumVersion)
			{
				delete $filterMap{$symbolName};
			}
		}
	}
	close $fh;
}


sub parseLibraries()
{
	my $counter = 0;
	my $dh;
	my $filename;
	my $dirname = $args{'libdir'};
	
	opendir($dh, $dirname) || die "Can't opendir $dirname: $!";
	while ($filename = readdir($dh))
	{
		next unless -f "$dirname/$filename";
		if (($counter++ % 50) == 0)
		{
			print STDOUT ".";
		}
		parseLibrary("$dirname/$filename");
	}
	closedir $dh;
}


sub appendSymbols($$)
{
	my ($output, $unavailableSymbols) = @_;
	my $minimumVersion = version_number($args{'target'});

	printf $output "\n";
	if ($unavailableSymbols)
	{
		printf $output "/* Symbols introduced in newer glibc versions, which must not be used */\n";
	} else
	{
		printf $output "/* Symbols redirected to earlier glibc versions */\n";
	}
	
	for my $it (sort { $a cmp $b } keys %symbolMap)
	{
		my $version = $symbolMap{$it};
		my $this_version = version_number($version);
		my $versionToUse;

		# If the symbol only has occurrences older than the minimum required glibc version,
		# then there is no need to output anything for this symbol
		next if exists($filterMap{$it});

		# If the only available symbol is > minimumVersion, then redirect it
		# to a comprehensible linker error, otherwise redirect the symbol
		# to it's existing version <= minimumVersion.
		if ($this_version > $minimumVersion)
		{
			$versionToUse = sprintf("DONT_USE_THIS_VERSION_%s", $version);
			next if (!$unavailableSymbols);
		} else
		{
			$versionToUse = $version;
			next if ($unavailableSymbols);
		}

		printf $output 'SYMVER(%s, GLIBC_%s)' . "\n", $it, $versionToUse;
	}
}


sub generateHeader()
{
	my $output;
	my $filename = $args{'output'};
	
	open($output, ">", $filename) || die "Can't open $filename: $!";
	
	# FIXME: Currently we do:
	#
	#   if !defined (__OBJC__) && !defined (__ASSEMBLER__)
	#
	# But what we want is a clause which accepts any form of C including C++ and probably
	# also including ObjC. That said, the generated header works fine for C and C++ sources.
	printf $output "/* glibc bindings for target ABI version glibc %s */\n", $args{'target'};
	printf $output "#if defined(__linux__) && !defined (__LIBC_CUSTOM_BINDINGS_H__) && !defined(__ANDROID__)\n";
	printf $output "\n";
	printf $output "#if defined (__cplusplus)\n";
	printf $output "extern \"C\" {\n";
	printf $output "#endif\n";
	printf $output "\n";
	printf $output "#undef SYMVER\n";
	printf $output "#undef SYMVER1\n";
	printf $output "#ifdef __ASSEMBLER__\n";
	printf $output "#define SYMVER1(name, ver) .symver name, name##@##ver\n";
	printf $output "#else\n";
	printf $output "#define SYMVER1(name, ver) __asm__(\".symver \" #name \", \" #name \"@\" #ver );\n";
	printf $output "#endif\n";
	printf $output "#define SYMVER(name, ver) SYMVER1(name, ver)\n";
	printf $output "\n";

	# First generate the available redirected symbols, then the unavailable symbols
	appendSymbols($output, 0);
	appendSymbols($output, 1);

	printf $output "\n";
	printf $output "#undef SYMVER\n";
	printf $output "#undef SYMVER1\n";
	printf $output "\n";
	printf $output "#if defined (__cplusplus)\n";
	printf $output "}\n";
	printf $output "#endif\n";
	printf $output "#endif\n";

	close($output);
}


STDOUT->autoflush(1);
STDERR->autoflush(1);

GetOptions ("libdir|L=s" => \$args{libdir},
            "output|o=s" => \$args{output},
            "target|t=s" => \$args{target},
            "man" => \$args{man},
            "help|?|h" => \$args{help},
            "version|V" => \$args{version},
           ) or usage(2);

usage(0) if $args{help};
man() if $args{man};
print_version_and_exit() if $args{version};

if (!defined($args{'libdir'}))
{
	usage_error("Must specify --libdir\n");
}

if (!defined($args{'output'}))
{
	usage_error("Must specify --output\n");
}


eval {
	printf "Generating %s (glibc %s) from libs at '%s' ", $args{'output'}, $args{'target'}, $args{'libdir'};

	parseLibraries();
	generateHeader();
};
if (my $e = $@) {
	print STDERR $e;
	exit 1;
}

print " OK\n";

exit 0;

__END__

=head1 NAME

=encoding utf8

X<me> - generate header file for targetting specific glibc ABI

=head1 SYNOPSIS

X<me> [<options>]

Options:

=over

=item -L, --libdir <dir>                  Library directory

=item -t, --target <MAJOR.MINOR[.MICRO]>  Target glibc ABI (Default: X<default_target>)

=item -o, --output <filename>             Header to create (Default: X<output>)

=back

=head1 AUTHORS

=over

=item Jan Niklas Hasse <jhasse@gmail.com>

=item Upstairs Laboratories Inc.

=item Thorsten Otto

=back

=cut
