#!/usr/bin/perl
# Read include file, to generate loader

$file = @ARGV[0];

if ( ! defined(open(FILE, $file)) ) {
	warn "Couldn't open $file: $!\n";
	exit;
}

print "/* Generated by dyngl_c.pl from $file */\n\n";

$linecount=0;
while ($line = <FILE>) {
	if ($line =~ /^GLAPI/ ) {

		$return_type = "" ;
		$function_name = "" ;
		if ($line =~ /^GLAPI *(\w+).* G?L?APIENTRY *gl(\w+) *\(.*/) {
			$return_type = $1 ;
			$function_name = $2 ;
		}

		$line =~ s/GLAPI *// ;
		$line =~ s/ *GLAPIENTRY// ;
		$line =~ s/ *APIENTRY// ;
		$line =~ s/;$// ;
		chomp($line);

		print "\tregl.$function_name = SDL_GL_GetProcAddress(\"gl$function_name\");\n";

		$linecount++;
	}
}
close(FILE);
print "/* Functions generated : $linecount */\n";