#!/usr/bin/perl

use File::Spec;

my $out = shift @ARGV or die;
my $root = shift @ARGV or die;
($volume,$dir,$file) = File::Spec->splitpath($root);

open OUT, ">$out" or die "$out: $!\n";
my %done;

sub inline
{
	my ($name) = @_;
	return 0 if ($done{$name});
	$done{$name} = 1;
	print " - $name\n";
	print OUT "//////// Start of inlined file: $name ////////\n\n";
	open my $IN, $name or die "$name: $!\n";
	while (<$IN>) {
		if (/^#include \"(.+)\"/ && -f $1 && !inline($1)) {
			print OUT "//$_";
		} else {
			print OUT "$_";
		}
	}
	close $IN;
	print OUT "\n//////// End of inlined file: $name ////////\n\n";
	return 1;
}

if(!$volume.$dir eq "")
{
	chdir $volume.$dir;
}
inline($file);

close OUT;
