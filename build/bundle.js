var app=function(){"use strict";function t(){}function n(t){return t()}function e(){return Object.create(null)}function o(t){t.forEach(n)}function r(t){return"function"==typeof t}function c(t,n){return t!=t?n==n:t!==n||t&&"object"==typeof t||"function"==typeof t}let l,i=!1;function u(t,n,e,o){for(;t<n;){const r=t+(n-t>>1);e(r)<=o?t=r+1:n=r}return t}function a(t,n){i?(!function(t){if(t.hydrate_init)return;t.hydrate_init=!0;const n=t.childNodes,e=new Int32Array(n.length+1),o=new Int32Array(n.length);e[0]=-1;let r=0;for(let t=0;t<n.length;t++){const c=u(1,r+1,(t=>n[e[t]].claim_order),n[t].claim_order)-1;o[t]=e[c]+1;const l=c+1;e[l]=t,r=Math.max(l,r)}const c=[],l=[];let i=n.length-1;for(let t=e[r]+1;0!=t;t=o[t-1]){for(c.push(n[t-1]);i>=t;i--)l.push(n[i]);i--}for(;i>=0;i--)l.push(n[i]);c.reverse(),l.sort(((t,n)=>t.claim_order-n.claim_order));for(let n=0,e=0;n<l.length;n++){for(;e<c.length&&l[n].claim_order>=c[e].claim_order;)e++;const o=e<c.length?c[e]:null;t.insertBefore(l[n],o)}}(t),(void 0===t.actual_end_child||null!==t.actual_end_child&&t.actual_end_child.parentElement!==t)&&(t.actual_end_child=t.firstChild),n!==t.actual_end_child?t.insertBefore(n,t.actual_end_child):t.actual_end_child=n.nextSibling):n.parentNode!==t&&t.appendChild(n)}function s(t,n,e){i&&!e?a(t,n):(n.parentNode!==t||e&&n.nextSibling!==e)&&t.insertBefore(n,e||null)}function f(t){t.parentNode.removeChild(t)}function d(t,n){for(let e=0;e<t.length;e+=1)t[e]&&t[e].d(n)}function h(t){return document.createElement(t)}function p(t){return document.createTextNode(t)}function g(){return p(" ")}function m(t,n,e,o){return t.addEventListener(n,e,o),()=>t.removeEventListener(n,e,o)}function $(t,n){n=""+n,t.wholeText!==n&&(t.data=n)}function _(t,n){t.value=null==n?"":n}function y(t){l=t}const v=[],b=[],w=[],x=[],k=Promise.resolve();let E=!1;function N(t){w.push(t)}let j=!1;const S=new Set;function O(){if(!j){j=!0;do{for(let t=0;t<v.length;t+=1){const n=v[t];y(n),A(n.$$)}for(y(null),v.length=0;b.length;)b.pop()();for(let t=0;t<w.length;t+=1){const n=w[t];S.has(n)||(S.add(n),n())}w.length=0}while(v.length);for(;x.length;)x.pop()();E=!1,j=!1,S.clear()}}function A(t){if(null!==t.fragment){t.update(),o(t.before_update);const n=t.dirty;t.dirty=[-1],t.fragment&&t.fragment.p(t.ctx,n),t.after_update.forEach(N)}}const B=new Set;function C(t,n){-1===t.$$.dirty[0]&&(v.push(t),E||(E=!0,k.then(O)),t.$$.dirty.fill(0)),t.$$.dirty[n/31|0]|=1<<n%31}function M(c,u,a,s,d,h,p=[-1]){const g=l;y(c);const m=c.$$={fragment:null,ctx:null,props:h,update:t,not_equal:d,bound:e(),on_mount:[],on_destroy:[],on_disconnect:[],before_update:[],after_update:[],context:new Map(g?g.$$.context:u.context||[]),callbacks:e(),dirty:p,skip_bound:!1};let $=!1;if(m.ctx=a?a(c,u.props||{},((t,n,...e)=>{const o=e.length?e[0]:n;return m.ctx&&d(m.ctx[t],m.ctx[t]=o)&&(!m.skip_bound&&m.bound[t]&&m.bound[t](o),$&&C(c,t)),n})):[],m.update(),$=!0,o(m.before_update),m.fragment=!!s&&s(m.ctx),u.target){if(u.hydrate){i=!0;const t=function(t){return Array.from(t.childNodes)}(u.target);m.fragment&&m.fragment.l(t),t.forEach(f)}else m.fragment&&m.fragment.c();u.intro&&((_=c.$$.fragment)&&_.i&&(B.delete(_),_.i(v))),function(t,e,c,l){const{fragment:i,on_mount:u,on_destroy:a,after_update:s}=t.$$;i&&i.m(e,c),l||N((()=>{const e=u.map(n).filter(r);a?a.push(...e):o(e),t.$$.on_mount=[]})),s.forEach(N)}(c,u.target,u.anchor,u.customElement),i=!1,O()}var _,v;y(g)}function T(t,n,e){const o=t.slice();return o[12]=n[e],o}function I(t,n,e){const o=t.slice();return o[15]=n[e],o}function J(t){let n,e,o,r,c,l,i,u,d,m=t[15].当前路径.map(q).join("/")+"",_=t[15].目前指令+"",y=t[15].指令结果+"";return{c(){n=p("----------------------------------------------------\n  "),e=h("div"),o=p(m),r=g(),c=h("div"),l=p(_),i=g(),u=h("div"),d=p(y)},m(t,f){s(t,n,f),s(t,e,f),a(e,o),s(t,r,f),s(t,c,f),a(c,l),s(t,i,f),s(t,u,f),a(u,d)},p(t,n){2&n&&m!==(m=t[15].当前路径.map(q).join("/")+"")&&$(o,m),2&n&&_!==(_=t[15].目前指令+"")&&$(l,_),2&n&&y!==(y=t[15].指令结果+"")&&$(d,y)},d(t){t&&f(n),t&&f(e),t&&f(r),t&&f(c),t&&f(i),t&&f(u)}}}function L(t){let n,e,o=JSON.stringify(t[12])+"";return{c(){n=h("div"),e=p(o)},m(t,o){s(t,n,o),a(n,e)},p(t,n){2&n&&o!==(o=JSON.stringify(t[12])+"")&&$(e,o)},d(t){t&&f(n)}}}function P(n){let e,r,c,l,i,u,y,v,b,w=n[1],x=[];for(let t=0;t<w.length;t+=1)x[t]=J(I(n,w,t));let k=n[1],E=[];for(let t=0;t<k.length;t+=1)E[t]=L(T(n,k,t));return{c(){for(let t=0;t<x.length;t+=1)x[t].c();e=g(),r=h("div"),c=h("input"),l=g(),i=p(n[2]),u=g();for(let t=0;t<E.length;t+=1)E[t].c();var t,o,a;y=p(""),t="width",o="100%",c.style.setProperty(t,o,a?"important":"")},m(t,o){for(let n=0;n<x.length;n+=1)x[n].m(t,o);s(t,e,o),s(t,r,o),a(r,c),_(c,n[0]),a(r,l),a(r,i),s(t,u,o);for(let n=0;n<E.length;n+=1)E[n].m(t,o);s(t,y,o),v||(b=[m(window,"keydown",n[3]),m(c,"input",n[5])],v=!0)},p(t,[n]){if(2&n){let o;for(w=t[1],o=0;o<w.length;o+=1){const r=I(t,w,o);x[o]?x[o].p(r,n):(x[o]=J(r),x[o].c(),x[o].m(e.parentNode,e))}for(;o<x.length;o+=1)x[o].d(1);x.length=w.length}if(1&n&&c.value!==t[0]&&_(c,t[0]),4&n&&$(i,t[2]),2&n){let e;for(k=t[1],e=0;e<k.length;e+=1){const o=T(t,k,e);E[e]?E[e].p(o,n):(E[e]=L(o),E[e].c(),E[e].m(y.parentNode,y))}for(;e<E.length;e+=1)E[e].d(1);E.length=k.length}},i:t,o:t,d(t){d(x,t),t&&f(e),t&&f(r),t&&f(u),d(E,t),t&&f(y),v=!1,o(b)}}}const q=t=>t.name;function z(t,n,e){class o{constructor(t,n){this.name=t,this.content=null!=n?n:""}}class r{constructor(t){this.name=t,this.children=[]}}let c=new r("~"),l=[c],i=new Map([["ls",t=>l[l.length-1].children.map((t=>t.name)).join(" ")],["mkdir",t=>(l[l.length-1].children.push(new r(t[0])),"")],["cd",t=>{let n=t[0];if(".."===n)return l.length>1?(l.pop(),""):"can't cd to path";let e=l[l.length-1].children.find((t=>t instanceof r&&t.name===n));return void 0!==e&&e instanceof r?(l.push(e),console.log("新建成功"),""):"can't cd to path"}],["touch",t=>(l[l.length-1].children.push(...t.map((t=>new o(t)))),"创建完成")],["cat",t=>{let n=t[0],e=l[l.length-1].children.find((t=>t.name===n&&t instanceof o));return void 0!==e&&e instanceof o?e.content:`cat: ${n}: 没有那个文件`}],["echo",t=>{let n=t[0],e=t[2],r=l[l.length-1].children.find((t=>t.name===e&&t instanceof o));if(void 0===r)return l[l.length-1].children.push(new o(e,n)),"";if(r instanceof o){if(">"===t[1])return r.content=n,"";if(">>"===t[1])return r.content+=n,""}}]]);let u="",a=[{"目前指令":"","指令结果":"","当前路径":[c]}],s="";return t.$$.update=()=>{16&t.$$.dirty&&(e(4,l),l.map((t=>t.name)).join("/"),console.log(l))},[u,a,s,t=>{if("Enter"===t.key){if(""===u)return;e(2,s=function(t){let n=t.split(" "),e=n[0],o=n.slice(1);return new Set(i.keys()).has(e)?i.get(e)(o):"error, 无此指令"}(u)),e(1,a[a.length-1].目前指令=u,a),e(1,a[a.length-1].指令结果=s,a),a.push({"目前指令":"","指令结果":"","当前路径":l.concat()}),e(0,u=""),e(2,s="")}if("Tab"===t.key){let t=u.split(" ");t[t.length-1]}console.log(t.key)},l,function(){u=this.value,e(0,u)}]}return new class extends class{$destroy(){!function(t,n){const e=t.$$;null!==e.fragment&&(o(e.on_destroy),e.fragment&&e.fragment.d(n),e.on_destroy=e.fragment=null,e.ctx=[])}(this,1),this.$destroy=t}$on(t,n){const e=this.$$.callbacks[t]||(this.$$.callbacks[t]=[]);return e.push(n),()=>{const t=e.indexOf(n);-1!==t&&e.splice(t,1)}}$set(t){var n;this.$$set&&(n=t,0!==Object.keys(n).length)&&(this.$$.skip_bound=!0,this.$$set(t),this.$$.skip_bound=!1)}}{constructor(t){super(),M(this,t,z,P,c,{})}}({target:document.body})}();
//# sourceMappingURL=bundle.js.map