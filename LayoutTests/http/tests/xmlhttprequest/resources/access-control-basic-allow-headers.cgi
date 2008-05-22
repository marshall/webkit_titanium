#!/usr/bin/perl -wT
use strict;

print "Content-Type: text/plain\n";
print "Access-Control: allow <http://127.0.0.1:8000>\n\n";

print "PASS: Cross-domain access allowed.\n";
foreach (keys %ENV) {
    if ($_ =~ "HTTP_") {
        print $_ . ": " . $ENV{$_} . "\n";
    }
}