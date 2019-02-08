#!/usr/bin/perl -c

# Package to handle Cyrus Header files

package Cyrus::HeaderFile;

use strict;
use warnings;

use IO::File;
use IO::File::fcntl;
use IO::Handle;
use File::Temp;
use Data::Dumper;

=pod

=head1 NAME

Cyrus::HeaderFile - A pure perl interface to the "cyrus.header" file
format as generated by Cyrus IMAPd.

=head1 EXAMPLES

Like Cyrus::IndexFile, uses fcntl locking (default for Cyrus on systems
which support it)

my $header = Cyrus::HeaderFile->new_file("path/to/cyrus.header");

XXX: see index_uids.pl

=cut

our $HL1 = qq{\241\002\213\015Cyrus mailbox header};
our $HL2 = qq{"The best thing about this system was that it had lots of goals."};
our $HL3 = qq{\t--Jim Morris on Andrew};

=head1 PUBLIC API

=over

=item Cyrus::HeaderFile->new($fh)

Read the header file in $fh

=cut

sub new {
  my $class = shift;
  my $handle = shift;

  # read header
  local $/ = undef;
  my $body = <$handle>;

  my $Self = bless {}, ref($class) || $class;
  $Self->{handle} = $handle; # keep for locking
  $Self->{rawheader} = $body;
  $Self->{header} = $Self->parse_header($body);

  return $Self;
}

=item Cyrus::HeaderFile->new_file($fname, $lockopts)

Open the file to read, optionally locking it with IO::File::fcntl.  If you
pass a scalar for lockopts then it will be locked with ['lock_ex'], otherwise
you can pass a tuple, e.g. ['lock_ex', 5] for a 5 second timeout.

This function will die if it can't open or lock the file.  On success, it
calls $class->new() with the filehandle.

=cut

sub new_file {
  my $class = shift;
  my $file = shift;
  my $lockopts = shift;

  my $fh;
  if ($lockopts) {
    $lockopts = ['lock_ex'] unless ref($lockopts) eq 'ARRAY';
    $fh = IO::File::fcntl->new($file, '+<', @$lockopts)
          || die "Can't open $file for locked read: $!";
  } else {
    $fh = IO::File->new("< $file")
          || die "Can't open $file for read: $!";
  }

  return $class->new($fh);
}

=item $header->header([$Field])

Return the entire header as a hash, or individual named field.

=cut

sub header {
  my $Self = shift;
  my $Field = shift;

  if ($Field) {
    return $Self->{header}{$Field};
  }

  return $Self->{header};
}

=item $header->write_header($fh, $headerData)

Write a header file with the data (e.g. returned from ->header())
to the given filehandle.

=cut

sub write_header {
  my $Self = shift;
  my $fh = shift;
  my $header = shift || $Self->header();

  $fh->print($Self->make_header($header));
}

sub make_header {
  my $Self = shift;
  my $ds = shift || $Self->header();

  # NOTE: acl and flags should have '' as the last element!
  my $flags = join(" ", @{$ds->{Flags}}, '');
  my $acl = join("\t", @{$ds->{ACL}}, '');
  my $buf = <<EOF;
$HL1
$HL2
$HL3
$ds->{QuotaRoot}        $ds->{UniqueId}
$flags
$acl
EOF
  return $buf;
}

sub parse_header {
  my $Self = shift;
  my $body = shift;

  my @lines = split /\n/, $body;

  die "Not a mailbox header file" unless $lines[0] eq $HL1;
  die "Not a mailbox header file" unless $lines[1] eq $HL2;
  die "Not a mailbox header file" unless $lines[2] eq $HL3;
  my ($quotaroot, $uniqueid) = split /\t/, $lines[3];
  my (@flags) = split / /, $lines[4];
  my (@acl) = split /\t/, $lines[5];

  return {
    QuotaRoot => $quotaroot,
    UniqueId => $uniqueid,
    Flags => \@flags,
    ACL => \@acl,
  };
}

=back

=head1 AUTHOR AND COPYRIGHT

Bron Gondwana <brong@fastmail.fm> - Copyright 2008 FastMail

Licenced under the same terms as Cyrus IMAPd.

=cut


1;
