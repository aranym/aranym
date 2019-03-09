#!/usr/bin/env perl

# Purpose: Purge old snapshots/pullrequests

require 5.008;
use strict;
use warnings;
use JSON;
use LWP::UserAgent;
use Data::Dumper;
use MIME::Base64;
use HTTP::Date;

die "error: BINTRAY_API_KEY is undefined" unless defined($ENV{BINTRAY_API_KEY});
my $BINTRAY_API_KEY = $ENV{BINTRAY_API_KEY};

my $BINTRAY_HOST = 'https://api.bintray.com';
my $BINTRAY_USER = 'aranym';
$BINTRAY_USER = $ENV{BINTRAY_USER} if defined($ENV{BINTRAY_USER});
my $BINTRAY_REPO_OWNER = $BINTRAY_USER;
$BINTRAY_REPO_OWNER = $ENV{BINTRAY_REPO_OWNER} if defined($ENV{BINTRAY_REPO_OWNER});
my $BINTRAY_REPO = 'aranym-files';
$BINTRAY_REPO = $ENV{BINTRAY_REPO_NAME} if defined($ENV{BINTRAY_REPO_NAME});
$BINTRAY_REPO ="${BINTRAY_REPO_OWNER}/${BINTRAY_REPO}";

# Number of days of retention before purge
my $days_retention = 7;

# But always keep a minimal number of snapshots
my $min_keep = 5;

my $json = JSON->new();

my $ua = LWP::UserAgent->new('keep_alive' => 1);
$ua->default_header('Accept', 'application/json');
$ua->default_header('Authorization', 'Basic ' . MIME::Base64::encode("$BINTRAY_USER:$BINTRAY_API_KEY", ''));

sub bintray_get($)
{
	my ($url) = @_;
	my $res = $ua->get("${BINTRAY_HOST}$url");
	die "$url: " . $res->status_line unless ($res->is_success);
	return $json->decode($res->decoded_content);
}

sub bintray_delete($)
{
	my ($url) = @_;
	my $res = $ua->delete("${BINTRAY_HOST}$url");
	die "$url: " . $res->status_line unless ($res->is_success);
	return $json->decode($res->decoded_content);
}

sub bintray_purge($)
{
	my ($package) = @_;
	
	#my $packages = bintray_get("/repos/${BINTRAY_REPO}/packages");
	#print Dumper(\$packages);
	
	my $snapshots = bintray_get("/packages/${BINTRAY_REPO}/${package}?attribute_values=1");
	#print Dumper(\$snapshots);
	
	# Every snapshot up to this day will be purged
	my $last_time_purge = time() - $days_retention * 24 * 60 * 60;
	
	# Current number of snapshots kept
	my $n = 0;
	
	foreach my $version (@{$snapshots->{'versions'}})
	{
		$version = bintray_get("/packages/${BINTRAY_REPO}/${package}/versions/${version}?attribute_values=1");
		#print Dumper(\$version);

		# This is one more snapshot
		++$n;
		
		# Do not purge until minimal number of snapshots
		if ($n <= $min_keep)
		{
			print "keep $version->{name}\n";
			next;
		}
		
		# Purge old snapshots
		my $released = HTTP::Date::str2time($version->{released});
		if ($released < $last_time_purge)
		{
			print "rm $version->{name}\n";
			bintray_delete("/packages/${BINTRAY_REPO}/${package}/versions/$version->{name}");
		} else
		{
			print "keep $version->{name}\n";
		}
	}
}

if (defined($ENV{TRAVIS_PULL_REQUEST}) && $ENV{TRAVIS_PULL_REQUEST} ne 'false')
{
	bintray_purge('pullrequests');
} else
{
	bintray_purge('snapshots');
	bintray_purge('temp');
}

exit 0;

__END__
