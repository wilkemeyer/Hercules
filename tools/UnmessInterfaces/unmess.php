<?php

// 
// Fixes function declarations as well as function calls by 
// its class names (interface -> class)
//
//define('WRITE_CHANGES', FALSE);
 
$g_messdef = array();	// index = filename (including path)

 
function readMessDEF(){
	global $g_messdef;
	$fp = fopen("./messdef.txt", "r");
	
	if(!$fp) die("Cannot open messdef.txt");
	
	// syntax:
	//  // = comment
	//  !folder name
	//	 bla#bla_interface: 
	
	$curFolder = '';
	$curClass = '';
	$curInterface = '';
	
	$linecnt = 0;
	while(1){
		$linecnt++;
		
		$line = fgets($fp, 8192);
		if($line === null || $line === false)
			break;
			
		$line = trim($line);
		
		$llen = strlen($line);
		if($llen === 0 || ($line[0] === '/' && $line[1] === '/') || ($line[0] === '/' && $line[1] === '*')){
			continue; // skip as its empty or comment
		}
		
		
		if($line[0] === '='){
			//
			if($line[1] === '1'){
				define('WRITE_CHANGES', TRUE);
				print "WRITE_CHANGES = TRUE!\n";
			}else{
				define('WRITE_CHANGES', FALSE);
				print "WRITE_CHANGES = FALSE!\n";
			}
		
		}else if($line[0] === '!'){
			$curFolder = substr($line, 1);
			
		}else if($line[$llen-1] === ':'){
			$line = substr($line, 0, -1);
			$llen--;
			
			$hPos = strpos($line, '#');
			if($hPos !== FALSE){
					$curClass = substr($line, 0, $hPos);
					$curInterface = substr($line, $hPos+1);

			}else{
				//..
				$curClass = $line;
				$curInterface = "${curClass}_interface";
				
			}

		}else{
			// declratation
			$a = explode('=', $line);
			if(sizeof($a) !== 2){
				print "messdef.txt:$linecnt -> malformed definition\n";
				continue;
			}
			
			$alias = explode('->', trim($a[0]));
			$realfunc = trim($a[1]);
			
			// remove colon at function name
			if($realfunc[strlen($realfunc)-1] === ';')
				$realfunc = substr($realfunc, 0, -1);
			
			//
			$SrcFilePath = "$curFolder/$curClass";
			if(array_key_exists($SrcFilePath, $g_messdef) === false){
				$g_messdef[$SrcFilePath] = array();
			}
			$refMess = &$g_messdef[$SrcFilePath];
			
			$cclass = 'C'.$alias[0];
			$cclass[1] = strtoupper($cclass[1]);
			
			$refMess[] = array( 'interface' => $curInterface,
								'CFunc' => $realfunc,
								'oldclass' => $alias[0],
								'class' => $cclass,
								'method' => $alias[1]);
			
		}
		
	}//endwhile
	fclose($fp);

 }
 
 
 function processHeader($file, &$def){
	// Todo here:
	// 	- fix interface delaration
	//	 In Detail:
	// 		- Rename struct bla_interface  to   class bla 
	//      - add public: below class declaration
	//		- change function pointers to static method declrations
	//
	
	$interface = $def[0]['interface'];
	$class = $def[0]['class'];
	$oldclass = $def[0]['oldclass'];
	
	$nReplacements = 0;
	
	$fp = fopen($file, 'r');
	if(!$fp) die("$file open failed!\n");

	$ls = array();	// line stack
	$linecnt=0;
	$inStruct = false;
	while(1){		
		$linecnt++;
		$line = fgets($fp, 8192);
		if($line === false || $line === null)
			break;
		
		// parseline
		$pLine = trim($line);
		
		if(strpos($line, 'struct') !== FALSE && 
			strpos($line, $interface) !== FALSE){
				// found Interface declarator
				// replace with class
				
				// check if its the export 
				if(strpos($line, 'HPShared') !== FALSE){
						// strip out.
						// may be a bit dirty but as every interface external / export is defined as HPShared 
						// this will work.
										
						
				}else{ 
					$inStruct = true;
					
					$ls[] = "class $class {\n";
					$ls[] = "public:\n";
				}
		}else if($inStruct === true){
			if($pLine === '};'){
				$inStruct = false;
				$ls[] = $line;
				
			}else{
				//
				//
				//
				$rpl = null;
				foreach($def as $n => $fncdef){
						
						// search for method:
						$decl = "(*${fncdef['method']})";
						
						if(strpos($line, $decl) !== FALSE){
							
							// replace by
							if($line[0] === "\t")
								$rpl = substr($line, 1);
							else
								$rpl = $line;
							
							//$rpl = "\tstatic ";
							$rpl = str_replace($decl, $fncdef['method'], $rpl);						
							$rpl = "\tstatic $rpl";
							
							$nReplacements++;
							break;
						}
						
				}//end foreach:
				
				if($rpl === null)
					$rpl = $line;
				
				$ls[] = $rpl;
				
				
			}
		}else{
			// unprocessed line -> add to linestack
			$ls[] = $line;
			
		}
		
		
		
	}
		
	fclose($fp);
	
	
	printf("%-28s %u of %u Definitions Converted\n", $file, $nReplacements, sizeof($def));
	
	if(WRITE_CHANGES && $nReplacements > 0){
		file_put_contents($file, implode($ls));
	}
	// 
	
 }//end: processHeader()
 
 
 function processSource($file, &$def){
	// Todo here:
	//	- fix function declarations
	//		timer_add -> timer::add
	//	- fix internal function calls by replacing
	//		timer_add with timer::add
	//
	
	$fp = fopen($file, 'r');
	if(!$fp) die("Cannot open $file\n");
	
	$ls = array();
	$nReplacements = 0;
	
	$linecnt = 0;
	while(1){
		$linecnt++;
		$line = fgets($fp, 8192);
		if($line === false || $line === null)
			break; //eof.
			
		$rpl = null;
		// 
		foreach($def as $idx => &$fncdef){
			$decl = array();
			$decl[] = "${fncdef['CFunc']}(";
			$decl[] = "${fncdef['CFunc']} (";
			$decl[] = "${fncdef['CFunc']}  (";
			// ~ the dirty way. :/
			
			for($i = 0; $i < sizeof($decl); $i++){
				if(strpos($line, $decl[$i]) !== FALSE){
					
					$cppdef = "${fncdef['class']}::${fncdef['method']}(";
					
					
					
					$rpl = str_replace($decl[$i], $cppdef, $line);
					////print "$line -> $rpl\n\n";
					$nReplacements++;
					break 2;
				}
			}
		}
		
		if($rpl === null)
			$ls[] = $line;
		else
			$ls[] = $rpl;
		
	}
	
	fclose($fp);

	printf("%-28s %u Matches\n", $file, $nReplacements);
	
	if(WRITE_CHANGES && $nReplacements > 0){
		file_put_contents($file, implode($ls));
	}
	
 }//end: processSource()
 
 
 
 
 // main
 readMessDEF();
 
foreach($g_messdef as $srcFile => &$def){	
	$header = "${srcFile}.h";
	$source = "${srcFile}.c";
	
	if(sizeof($def) > 0){
		processHeader($header, $def);
		processSource($source, $def);
	}

}
 
 