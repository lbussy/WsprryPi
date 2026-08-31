"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path");
const root=path.resolve(__dirname,"..");
const view=fs.readFileSync(path.join(root,"data/views/config.php"),"utf8");
const script=fs.readFileSync(path.join(root,"data/index.js"),"utf8");
const styles=fs.readFileSync(path.join(root,"data/index.css"),"utf8");
const header=fs.readFileSync(path.join(root,"data/header.php"),"utf8");
assert.match(view,/Requested[\s\S]*Persisted[\s\S]*Boot configured[\s\S]*Active[\s\S]*Module reported[\s\S]*Reconciled[\s\S]*Boot ownership[\s\S]*Pending transaction[\s\S]*Fixed services[\s\S]*Endpoint[\s\S]*live_output[\s\S]*Development policy[\s\S]*Operation lifecycle[\s\S]*Product qualification/);
assert.match(view,/>Apply route and reboot</); assert.match(view,/>Cancel</);
assert.match(view,/role="status" aria-live="polite" aria-atomic="true"/);
for(const state of ["checking","active","reboot_required","applying","staged","mismatch","unavailable","rollback","rollback_required"])
 assert.match(script,new RegExp(`${state}:`),`missing ${state} state`);
assert.match(script,/body:JSON\.stringify\(\{operation, route:requested, generation:this\.generation\}\)/);
assert.match(script,/operation:"preflight"[\s\S]*operation:"apply-and-reboot"/);
assert.match(script,/if \(rp1RouteUi && rp1RouteUi\.visible\(\)\)[\s\S]*return;[\s\S]*scheduleAutosave\(\)/);
assert.match(script,/this\.developmentCompatible \? "Apply route and reboot" : "Check route"/);
assert.match(script,/rp1-development-policy/);
assert.match(view,/id="rp1-development-policy">Disabled/);
assert.match(view,/id="rp1-route-eligible">Unqualified/);
assert.doesNotMatch(view,/Package|Predecessor evidence|Module \/ ABI|Compatibility/);
assert.match(view,/provider is provisioned outside Wsprry Pi/);
assert.match(script,/window\.confirm/);
assert.match(script,/wsprrypi\.service and soapyremote-server\.service/);
assert.match(header,/'rp1RoutePath' => \$basePath \. '\/api\/rp1-gpclk-route'/);
assert.match(styles,/@media \(max-width: 575\.98px\)[\s\S]*\.rp1-route-actions > \.btn/);
console.log("rp1_route_ui_test passed");

// Execute the real controller without a browser or external dependencies.
const vm=require("node:vm");
const nodes=new Map();
function element(id){
 if(!nodes.has(id)) nodes.set(id,{dataset:{},setAttribute(){},disabled:false,hidden:false,textContent:""});
 return nodes.get(id);
}
let pin=20;
const context={window:{fetch:async()=>{throw Error("no fixture")},confirm:()=>true},
 document:{getElementById:element,querySelector:element},
 getTxPin:()=>pin,setTxPin:value=>{pin=value},
 $:selector=>({prop(name,value){element(selector.slice(1))[name]=value;return this},
 text(value){element(selector.slice(1)).textContent=value;return this}})};
vm.createContext(context);
vm.runInContext(script.slice(script.indexOf("const RP1_ROUTE_STATES"),script.indexOf("function initializeRp1RouteUi"))+
 ';globalThis.controller=new Rp1RouteUiController("/offline");',context);
(async()=>{
 const controller=context.controller;
 controller.render({profile:"runtime",ok:true,state:"runtime_inhibited",persisted:"GPIO20",active:"GPIO4",compatible:true});
 assert.equal(element("rp1-route-apply").textContent,"Switch route (output disabled)");
 let confirmations=0,requests=[];
 context.window.confirm=()=>{confirmations++;return true};
 controller.request=async()=>({ok:true,json:async()=>({ok:false,profile:"runtime",state:"runtime_recovery"})});
 await controller.applyAndReboot();
 assert.equal(confirmations,0,"failed preflight cannot request confirmation");
 controller.request=async(url,options)=>{requests.push(JSON.parse(options.body));throw Error("disconnected")};
 await controller.operate("switch");
 assert.equal(requests.length,1,"no automatic effect retries");
 assert.equal(element("rp1-route-state").dataset.state,"runtime_unknown");
 controller.select("GPIO20");
 assert.equal(element("rp1-route-apply").disabled,true,"selection cannot clear unknown completion");
 controller.render({profile:"runtime",ok:true,state:"runtime_recovery",persisted:"GPIO20",active:"GPIO4"});
 assert.equal(element("rp1-route-rollback").hidden,false);
 context.window.confirm=()=>false;
 await controller.operate("rollback");
 assert.equal(requests.length,1,"cancelled recovery has no effect");
 console.log("runtime route UI behavior: PASS");
})().catch(error=>{console.error(error);process.exitCode=1});
