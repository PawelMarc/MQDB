function g(x,y) { return x*y/30; };

//strungifies and puts object as string
function puto(db, key, val) {
    return put(db, key, JSON.stringify(val));
}

function getall(db) {
    var ret = []; 
    var i = 0; 
    var it = it_new(db);
    for(it_first(it); it_valid(it); it_next(it)) { 
        ret[i++] = {'key': it_key(it), 'val': it_val(it)}; 
    };
    return ret;
};

function getallo(db) {
    var ret = []; 
    var i = 0; 
    var it = it_new(db);
    for(it_first(it); it_valid(it); it_next(it)) { 
        ret[i++] = {'key': it_key(it), 'val': JSON.parse(it_val(it))}; 
    };
    return ret;
};

function getallf(db, f) {
    var ret = []; 
    var i = 0; 
    var it = it_new(db);
    for(it_first(it); it_valid(it); it_next(it)) { 
        ret[i++] = f(it_key(it), it_val(it)); 
    };
    return ret;
};

function redall(db, f, acc) {
    var ret = []; 
    var i = 0; 
    var it = it_new(db);
    for(it_first(it); it_valid(it); it_next(it)) { 
        acc = f(it_key(it), it_val(it), acc); 
    };
    return acc;
};

function delall(db) {
    //var ret = []; 
    //var i = 0; 
    var it = it_new(db);
    for(it_first(it); it_valid(it); it_next(it)) { 
        del(db, it_key(it)); 
    }
};

function bs_setsafe(bs1, pos) {
    var bs2 = bs_new();
    var bs_or = bs_new();
    bs_set(bs2, pos);
    //bs_makeSameSize(bs1,bs2);
    bs_logicalor(bs1, bs2, bs_or);
    return bs_or;
}

function bs_resetsafe(bs1, pos) {
    var bs2 = bs_new();
    var bsand = bs_new();
    bs_set(bs2, pos);
    bs_makeSameSize(bs1,bs2);
    bs_inplace_logicalnot(bs2);
    bs_logicaland(bs1, bs2, bsand);
    return bsand;
}

function getnbs(db, key) {
    var bs = bs_new();
    getbs(db, key, bs);
    return bs;
}

function bs_toarray(bs) {
    var a = [];
    var i=0;
    for(var it = bs_it_new(bs); !bs_it_isend(bs, it); bs_it_next(it), i++) {
        a[i] = bs_it_val(it);
    }
    return a;
}        

//*********************************************************
//BIT INDEX HELPER FUNCTIONS

 
//*****************************
//sets bit in bit index in position Num(key) with key = val
function setidx(db, key, val){

    var pos = Number(key);
    print("setidx, pos: "+ pos + " val:" + val);
    
    //print("setidx, pos: "+ pos);
    
    var bs = getnbs(db, val);
    //print("bs: "+bs_tostring(bs));
    
    //print("setidx, safe a");
    bs = bs_setsafe(bs, pos);
    //print("setidx, safe b");
    putbs(db, val, bs);
};

//*****************************
//resets bit in bit index in position Num(key) with key = val
//used to remove "pointer" to old value of the record
function resetidx(db, key, val){

    var pos = Number(key);
    print("resetidx, pos: "+ pos + " val:" + val);
    
    var bs = getnbs(db, val);
    //print("bs: "+bs_tostring(bs));
    
    //print("setidx, safe a");
    bs = bs_resetsafe(bs, pos);
    //print("setidx, safe b");
    putbs(db, val, bs);
};

//*****************************
//gets array of {key: k, val: v} objects for all values for given index
// db is data database, dbx is index database (where bit vectors are)
function getidx(db, dbx, val){

    var a = [];
    var i = 0;
    var bs = getnbs(dbx, val);
    //iterate all values in bs, they are all integers!
    for(var it = bs_it_new(bs); !bs_it_isend(bs, it); bs_it_next(it), i++) {
        var key = String(bs_it_val(it));
        var val = get(db, key);
        a[i] = {"key": key, "val": val};
    }
    return a;
};

//*****************************
//gets array of {key: k, val: v} objects for all values for given index array
//it gets records witch has all index values (and operation)
// db is data database, dbx is index database (where bit vectors are)
function getidxall(db, dbx, val_arr){

    var a = [];
    var i = 0;
    var bs = 0;
    
    //create bs bit index that is "and" intersection of all indexes for values in val_arr
    for(i=0; i<val_arr.length; i++) {
        var val = val_arr[i];
        var bs2 = getnbs(dbx, val);
        if (bs) {
            var bsand = bs_new();
            bs_logicaland(bs, bs2, bsand);
            bs = bsand;
        } else {
            bs = bs2;
        }
    }
        
    //iterate all values in bs, they are all integers!
    for(var it = bs_it_new(bs), i=0; !bs_it_isend(bs, it); bs_it_next(it), i++) {
        var key = String(bs_it_val(it));
        var val = get(db, key);
        a[i] = {"key": key, "val": val};
    }
    return a;
};

//********************************
//puts value to db and index to dbx
//removes old "pointer" from dbx if record has value
//key -- has to be number
//val -- can be anything
//fkey -- function that returnes index key from val (this key will be used as key in index db)
//fval -- transforms val into string to put into db, like JSON.stringify
//fkeyold -- like fkey but works on string in database, like combination of JSON.parse with fkey
 
function putwithidx(db, dbx, key, val, fkey, fval, fkeyold) {
    var put_val = fval(val);
    var idx_val = fkey(val);
    //firts take old value
    var old = get(db, key);
    if (old) {
        //if old exists remove old index value
        var idx_val_old = fkeyold(old);
        resetidx(dbx, key, idx_val_old);    
    }
    put(db, key , put_val);
    setidx(dbx, key, idx_val);
}

//********************************
//like putwithidx but fkey and fkeyold return array of keys to use for indexing

function putwithidxtags(db, dbx, key, val, fkey, fval, fkeyold) {
    var put_val = fval(val);
    var idx_val_arr = fkey(val);
    //firts take old value
    var old = get(db, key);
    if (old) {
        //if old exists remove old index value
        var idx_val_old_arr = fkeyold(old);
        print("idx_val_old_arr: " + JSON.stringify(idx_val_old_arr));
        for(var i=0; i<idx_val_old_arr.length; i++) {
            resetidx(dbx, key, idx_val_old_arr[i]);
        }    
    }
    
    put(db, key , put_val);
    print("idx_val_arr: " + JSON.stringify(idx_val_arr));
    for(var i=0; i<idx_val_arr.length; i++) {
        setidx(dbx, key, idx_val_arr[i]);
    }
}

//****************************************
//Index test functions

function irand(v) {
    return Math.floor((Math.random()*v)+1);
}

function test_index() {
    delall("db/test1");
    delall("db/test1_idx");
    
    for( var i=0; i<100; i++) {
        var k = String(irand(100));
        var o = {"k":"aaa", "v":"a"+k};
        put("db/test1", k , JSON.stringify(o));
        setidx("db/test1_idx", k, o.k);
    }

    for( var i=0; i<100; i++) {
        var k = String(irand(100));
        var o = {"k":"bbb", "v":"b"+k};
        put("db/test1", k , JSON.stringify(o));
        setidx("db/test1_idx", k, o.k);
    }

    for( var i=0; i<100; i++) {
        var k = String(irand(100000));
        var o = {"k":"ccc", "v":"c"+k};
        put("db/test1", k , JSON.stringify(o));
        setidx("db/test1_idx", k, o.k);
    }

    var a = getidx("db/test1","db/test1_idx","aaa"); 
    //print("getidx");
    
    return JSON.stringify(a);
}

function test_index2() {
    delall("db/test1");
    delall("db/test1_idx");
    
    for( var i=0; i<100; i++) {
        var k = String(irand(100));
        var o = {"k":"aaa", "v":"aaa"+k};
        putwithidx("db/test1", "db/test1_idx" , k, o, function (o) {return o.k; }, JSON.stringify, function (o) {return JSON.parse(o).k; });
    }

    for( var i=0; i<100; i++) {
        var k = String(irand(100));
        var o = {"k":"bbb", "v":"bbb"+k};
        putwithidx("db/test1", "db/test1_idx" , k, o, function (o) {return o.k; }, JSON.stringify, function (o) {return JSON.parse(o).k; });
    }

    for( var i=0; i<100; i++) {
        var k = String(irand(100));
        var o = {"k":"ccc", "v":"ccc"+k};
        putwithidx("db/test1", "db/test1_idx" , k, o, function (o) {return o.k; }, JSON.stringify, function (o) {return JSON.parse(o).k; });
    }

    var a = getidx("db/test1","db/test1_idx","aaa"); 
    //print("getidx");
    
    return JSON.stringify(a);
}

function test_index_tags() {
    delall("db/test1");
    delall("db/test1_idx");
    
    for( var i=0; i<100; i++) {
        var k = String(irand(100));
        var o = {"k":"aaa", "t": ["a1","a2"], "v":"aaa"+k};
        putwithidxtags("db/test1", "db/test1_idx" , k, o, function (o) {return o.t; }, JSON.stringify, function (o) {return JSON.parse(o).t; });
    }

    for( var i=0; i<100; i++) {
        var k = String(irand(100));
        var o = {"k":"bbb", "t": ["b1","b2"], "v":"bbb"+k};
        putwithidxtags("db/test1", "db/test1_idx" , k, o, function (o) {return o.t; }, JSON.stringify, function (o) {return JSON.parse(o).t; });
    }

    for( var i=0; i<100; i++) {
        var k = String(irand(100));
        var o = {"k":"ccc", "t": ["c1","c2"], "v":"ccc"+k};
        putwithidxtags("db/test1", "db/test1_idx" , k, o, function (o) {return o.t; }, JSON.stringify, function (o) {return JSON.parse(o).t; });
    }

    var a = getidx("db/test1","db/test1_idx","c1"); 
    //print("getidx");
    
    return JSON.stringify(a);
}

function test_index_tags2() {
    delall("db/test1");
    delall("db/test1_idx");
    
    for( var i=0; i<100; i++) {
        var k = String(irand(100));
        var o = {"k":"aaa", "t": ["a1","a2"], "v":"aaa"+k};
        putwithidxtags("db/test1", "db/test1_idx" , k, o, function (o) {return o.t; }, JSON.stringify, function (o) {return JSON.parse(o).t; });
    }

    for( var i=0; i<100; i++) {
        var k = String(irand(100));
        var o = {"k":"bbb", "t": ["b1","b2"], "v":"bbb"+k};
        putwithidxtags("db/test1", "db/test1_idx" , k, o, function (o) {return o.t; }, JSON.stringify, function (o) {return JSON.parse(o).t; });
    }

    for( var i=0; i<100; i++) {
        var k = String(irand(100));
        var o = {"k":"ccc", "t": ["c1","c2"], "v":"ccc"+k};
        putwithidxtags("db/test1", "db/test1_idx" , k, o, function (o) {return o.t; }, JSON.stringify, function (o) {return JSON.parse(o).t; });
    }

    var a = getidxall("db/test1","db/test1_idx",["a1","a2"]); 
    //print("getidx");
    
    return JSON.stringify(a);
}


function test_index_tags3(ind_arr) {
    delall("db/test1");
    delall("db/test1_idx");

    function put_next(ind, tag_arr) {   
        var k = String(irand(100));
        ind;
        tag_arr[ind] = String(ind);
        var o = {"k":"aaa", "t": tag_arr, "v":JSON.stringify(tag_arr)};
        putwithidxtags("db/test1", "db/test1_idx" , k, o, function (o) {return o.t; }, JSON.stringify, function (o) {return JSON.parse(o).t; });
    }

    var tag_arr = [];
    for( var i=0; i<10; i++) {        
        put_next(i,tag_arr);
    }

    //var a = getidxall("db/test1","db/test1_idx",["1","7"]);
    var a = getidxall("db/test1","db/test1_idx",ind_arr); 
    //print("getidx");
    
    return JSON.stringify(a);
}

function test_index_tags3_create() {
    delall("db/test1");
    delall("db/test1_idx");

    function put_next(ind, tag_arr) {   
        var k = String(irand(100));
        ind;
        tag_arr[ind] = String(ind);
        var o = {"k":"aaa", "t": tag_arr, "v":JSON.stringify(tag_arr)};
        putwithidxtags("db/test1", "db/test1_idx" , k, o, function (o) {return o.t; }, JSON.stringify, function (o) {return JSON.parse(o).t; });
    }

    var tag_arr = [];
    for( var i=0; i<10; i++) {        
        put_next(i,tag_arr);
    }

    return '{"ok":"true"}';
}

function test_index_tags3_get(ind_arr) {
    //var a = getidxall("db/test1","db/test1_idx",["1","7"]);
    var a;
    if (ind_arr[0] == "*") {
        a = getall("db/test1");
    } else { 
        a = getidxall("db/test1","db/test1_idx",ind_arr);
    } 
    //print("getidx");
    
    return JSON.stringify(a);
}


function test_index_tags4_create() {
    delall("db/test1");
    delall("db/test1_idx");

    var rand_tags = ["var", "int", "fun", "return", "function", "for", "else", "if", "end", "test"];
    
    function prep_tags() {
        var ret = [];
        var count = irand(10);
        for(var i=0; i<count; i++) {
            ret[i] = rand_tags[irand(rand_tags.length-1)];
        }
        return ret;    
    }
    
    function put_next() {   
        var k = String(irand(100));
        var tag_arr = prep_tags();
        print("new tags for: " + k + " tags: " + JSON.stringify(tag_arr)); 
        var o = {"k":"aaa", "t": tag_arr, "v":JSON.stringify(tag_arr)};
        print("we have o");
        putwithidxtags("db/test1", "db/test1_idx" , k, o, function (o) {return o.t; }, JSON.stringify, function (o) {return JSON.parse(o).t; });
    }

    for( var i=0; i<100; i++) {   
        put_next();
    }

    return '{"ok":"true"}';
}

function test_index_tags4_get(ind_arr) {
    //var a = getidxall("db/test1","db/test1_idx",["1","7"]);
    var a = getidxall("db/test1","db/test1_idx",ind_arr); 
    //print("getidx");
    
    return JSON.stringify(a);
}

function geto(db, key) {
   var str = get(db, key);
   if (str) {
      return JSON.parse(str);
   }
   return null;
}

function putvc(db, key, obj) {

   var ret = 0;
   var ver = obj._rev;
   var oldo = geto(db, key);
   if (!oldo || ver == oldo._rev) {
      obj._rev++;
      put(db, key, JSON.stringify(obj));
      ret = JSON.stringify({"ok":1, "rev":obj.version});
   } else {
      ret = JSON.stringify({"error":"wrong version"});
   }
         
   return ret;
}


//object for doing patch operations
var dmp = new diff_match_patch();

/* calculates diff between to texts and returns new object {line, ch} with new position */ 
function newCurPos(cur_pos, old_text, new_text) {
    var diffs = dmp.diff_main(old_text, new_text, true);
    var old_lines = old_text.split(/\r?\n/);
   
    var text_pos = 0;
    for (var i=0; i<cur_pos.line; i++) text_pos += old_lines[i].length+1;
    text_pos += cur_pos.ch;
    var new_text_pos = dmp.diff_xIndex(diffs, text_pos);
    var sliced_new_text = new_text.slice(0, new_text_pos);
    var new_lines = sliced_new_text.split(/\r?\n/);
    //msg("old text pos: " + text_pos + " new_text_pos: " + new_text_pos + " new_lines.length: " + new_lines.length);
    var ret_pos = new Object();
    ret_pos.line = new_lines.length-1;
    ret_pos.ch = new_lines[new_lines.length-1].length;
    return ret_pos;
}

var msg_text = "";

function msg(str) {
      msg_text += str + "<BR>";
}

function patch(obj, curr_obj) {
   
     var new_text = obj.text;
     //var curr_obj = geto(db, key);
     if (curr_obj && (curr_obj["_rev"] != obj["_rev"]) ) {
         //msg("curr_obj.patches.length : " + curr_obj.patches.length + " obj.patches.length : " + obj.patches.length);
         for (var i=obj.patches.length; i< curr_obj.patches.length; i++) {
            
            var patches = dmp.patch_fromText(curr_obj["patches"][i]);
            var results = dmp.patch_apply(patches, new_text);
            new_text = results[0];
            msg("Patching from" + i + "patch:" + curr_obj["patches"][i] + " new_text: " + new_text);
         }            
         
         obj.text = new_text;
     }
     return obj;
}

function add_patch(obj, curr_obj) {
   
   if (curr_obj && curr_obj.patches) {
      obj.patches = curr_obj.patches.slice();
   } else {
      obj.patches = new Array();
   }
   if (curr_obj) {
      var patch = dmp.patch_toText(dmp.patch_make(curr_obj.text, obj.text));
      //msg("curr: " + curr_obj.text + " obj: " + obj.text + " patch: " + patch);
      if (patch != "") {
         obj.patches.push(patch);
      }
   }
   return obj;
};

function putdiff(db, key, obj) {
   if (!obj.patches) obj.patches = [];
   
   var curr_obj = geto(db, key);
   if (curr_obj && (curr_obj._rev != obj._rev)) {
      obj = patch(obj, curr_obj);
      obj._rev = curr_obj._rev;
   } 
   
   obj = add_patch(obj, curr_obj);
   
   obj._rev++;
   put(db, key, JSON.stringify(obj));
      
   return obj;
}

function ajax_putdiff(db, key, obj) {
    obj = putdiff(db, key, obj);
    return JSON.stringify({"ok":1, "rev":obj._rev, "obj":obj, "msg": msg_text});
}

//*************************************************
// TEST
//****************************************
//DB test functions
function test_db_delall() {
    var db = "db/testdel";
    for (var i=0; i<1000; i++) {
        put(db, irand(1000), irand(1000));
    }
    var size_before = getall(db).length;
    delall(db);
    var size_after = getall(db).length;
    
    var o = {"before": size_before, "after": size_after};
    return JSON.stringify(o);
}    


//****************************************
//Bitset Test functions

function test_bitset() {
    var bitset1 = bs_new();
    var bitset2 = bs_new();
    var orbitset = bs_new();
    var andbitset = bs_new();
    var notbitset = bs_new();
    
    for (var i =0; i<1; i++) {
        bs_reset(bitset1);//.reset();
        bs_reset(bitset2); //.reset();
        bs_reset(andbitset); //.reset();
        
        bs_set(bitset1, 1);
        bs_set(bitset1, 2);
        bs_set(bitset1, 21);
       
        
        bs_set(bitset2, 1);
        bs_makeSameSize(bitset1, bitset2);
        //print("makeSS");
        
        //bitset2.set(3);
        //bitset2.set(1000);
        //bitset2.set(1007);
        //bitset2.set(100000);
        //bitset2.set(10000000-1);
        
        //bitset2.logicalnot(notbitset);
        bs_inplace_logicalnot(bitset2);
        //print("inplace...");
        
        //bitset1.logicalor(bitset2,orbitset);
        //bitset1.sparselogicaland(notbitset,andbitset);
        bs_logicaland(bitset1, bitset2, andbitset);
        //print("loand");
    }

    //print("bs1");
    return "bs1: " + bs_tostring(bitset1) + "\n" + "bs2: " + bs_tostring(bitset2) + "\nand: " + bs_tostring(andbitset);
}

function test_bitset_set_safe() {
    var bitset1 = bs_new();
    
    for (var i =0; i<1; i++) {
        bs_reset(bitset1);//.reset();
        
        bitset1 = bs_setsafe(bitset1, 1000);
        bitset1 = bs_setsafe(bitset1, 100);
        bitset1 = bs_setsafe(bitset1, 50);
        bitset1 = bs_setsafe(bitset1, 11);    
    }

    //print("bs1");
    return "bs1: " + bs_tostring(bitset1);
}

function test_bitset_set_safe() {
    var bitset1 = bs_new();
    
    for (var i =0; i<1; i++) {
        bs_reset(bitset1);//.reset();
        
        bitset1 = bs_setsafe(bitset1, 1000);
        bitset1 = bs_setsafe(bitset1, 1001);
        bitset1 = bs_setsafe(bitset1, 100);
        bitset1 = bs_setsafe(bitset1, 101);
        bitset1 = bs_setsafe(bitset1, 50);
        bitset1 = bs_setsafe(bitset1, 51);
        bitset1 = bs_setsafe(bitset1, 10);
        bitset1 = bs_setsafe(bitset1, 11);
        
        bitset1 = bs_resetsafe(bitset1, 1001);
        bitset1 = bs_resetsafe(bitset1, 101);
        bitset1 = bs_resetsafe(bitset1, 51);
        bitset1 = bs_resetsafe(bitset1, 11);
            
    }

    //print("bs1");
    return "bs1: " + bs_tostring(bitset1);
}

function test_bitset_set_safe2() {
    //delall("db/testbs");
    var bs = bs_new(); //getnbs("db/testbs", "aaa");
    var bs2 = bs_new();
    bs_set(bs, 1);
    bs_set(bs2,2);
    bs3 = bs_new();
    bs_logicalor(bs, bs2, bs3);
    //bs = bs3;
    //putbs("db/testbs", "aaa", bs);
    //bs = getnbs("db/testbs", "aaa");
    
    //bs = bs_setsafe(bs, 100);
    
    
    //putbs("db/testbs", "aaa", bs);
    //bs = getnbs("db/testbs", "aaa");
    
    //bs = bs_setsafe(bs, 20);
    
    
    return "x bs: " + bs_tostring(bs3);
}

function test_bitset_it() {
    var bitset1 = bs_new();
    
    for (var i =0; i<1; i++) {
        bs_reset(bitset1);//.reset();
        
        bitset1 = bs_setsafe(bitset1, 1000);
        bitset1 = bs_setsafe(bitset1, 100);
        bitset1 = bs_setsafe(bitset1, 50);
        bitset1 = bs_setsafe(bitset1, 11);    
    }

    var ret = "bs: ";
    //var it = bs_it_new(bitset1);
    //ret += bs_it_val(it);
    //print("bs1");
    return "bs1: " + bs_toarray(bitset1).toString();
}

function test_bitset2() {
    var bitset1 = bs_new();
    var bitset2 = bs_new();
    
    function incv() {
        return Math.floor((Math.random()*100)+1);
    }
    
    function test1() {
        bs_reset(bitset1);//.reset();
        bs_reset(bitset2); //.reset();
        
        //set random bits in bitset
        var v = incv();
        for (var i=0; i<50; i++) {  
            bs_set(bitset1, v);
            v += incv();
        }
               
        putbs("db/testbs", "bs1", bitset1);
        getbs("db/testbs", "bs1", bitset2);
        
        return "bs1: " + bs_tostring(bitset1) + "\n" + "bs2: " + bs_tostring(bitset2) + "eq: " + bs_eq(bitset1, bitset2) + "<BR>";
    }
    
    var ret = "";
    for (var i =0; i<100; i++) {
        ret += test1();
    }

    //print("bs1");
    return ret;
}
    


