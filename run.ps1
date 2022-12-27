Get-Content "./pwshbuild.conf" | foreach-object -begin {$h=@{}} -process { $k = [regex]::split($_,'=');
	if(($k[0].CompareTo("") -ne 0) -and ($k[0].StartsWith("[") -ne $True)) { $h.Add($k[0], $k[1])}}
& $h.exepathDebug -r -u -xyz project_generator.txt --config -XG -nr my_conf.src
