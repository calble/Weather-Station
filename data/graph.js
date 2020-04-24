function f(){
  t.forEach(e=>{
    var x = document.createElement("div");
    x.setAttribute("class","r");
    x.setAttribute("style","height:" + (e+1) + "px; margin-top: " + (51-e) + "px");
    document.querySelector("#ts").appendChild(x);
  });
  p.forEach(e=>{
    var x = document.createElement("div");
    x.setAttribute("class","b");
    x.setAttribute("style","height:" + (e+1) + "px; margin-top: " + (51-e) + "px");
    document.querySelector("#ts").appendChild(x);
  });
  
  document.getElementById("reset").addEventListener("submit",function(e){
	e.preventDefault(); 
	if(confirm("Are you sure you want to reset the historical data?")){
		  this.submit();
	};
  });
}