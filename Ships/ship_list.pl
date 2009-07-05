opendir (this_dir, ".") or die "Cant open directory\n";

my @dirs = readdir (this_dir);

foreach (@dirs)
{
  if (-f $_)
  {
    printf ("%s~\n", $_);
  }
}
printf ("\$~\n", $_);

