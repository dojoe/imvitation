#!/usr/bin/perl

# Convert an .obj+.mtl exported by Blender into a single .obj with only color information from the .mtl retained and at the right spot

while (<>) {
	if (/mtllib (.*)/) {
		%mtllib = ();
		open F, $1;
		while (<F>) {
			/newmtl (.*)/ and $mtlnm = $1;
			/Kd/ and $mtllib{$mtlnm} = $_;
		}
		close F;
	}
	/^v/ and $verts .= $_;
	/^f/ and print;
	if (/usemtl (.*)/) {
		print $mtllib{$1};
		print $verts;
		$verts = "";
	}
}