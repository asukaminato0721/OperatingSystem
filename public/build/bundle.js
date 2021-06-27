
(function(l, r) { if (l.getElementById('livereloadscript')) return; r = l.createElement('script'); r.async = 1; r.src = '//' + (window.location.host || 'localhost').split(':')[0] + ':35729/livereload.js?snipver=1'; r.id = 'livereloadscript'; l.getElementsByTagName('head')[0].appendChild(r) })(window.document);
var app = (function () {
    'use strict';

    function noop() { }
    function add_location(element, file, line, column, char) {
        element.__svelte_meta = {
            loc: { file, line, column, char }
        };
    }
    function run(fn) {
        return fn();
    }
    function blank_object() {
        return Object.create(null);
    }
    function run_all(fns) {
        fns.forEach(run);
    }
    function is_function(thing) {
        return typeof thing === 'function';
    }
    function safe_not_equal(a, b) {
        return a != a ? b == b : a !== b || ((a && typeof a === 'object') || typeof a === 'function');
    }
    function is_empty(obj) {
        return Object.keys(obj).length === 0;
    }

    function append(target, node) {
        target.appendChild(node);
    }
    function insert(target, node, anchor) {
        target.insertBefore(node, anchor || null);
    }
    function detach(node) {
        node.parentNode.removeChild(node);
    }
    function destroy_each(iterations, detaching) {
        for (let i = 0; i < iterations.length; i += 1) {
            if (iterations[i])
                iterations[i].d(detaching);
        }
    }
    function element(name) {
        return document.createElement(name);
    }
    function text(data) {
        return document.createTextNode(data);
    }
    function space() {
        return text(' ');
    }
    function empty() {
        return text('');
    }
    function listen(node, event, handler, options) {
        node.addEventListener(event, handler, options);
        return () => node.removeEventListener(event, handler, options);
    }
    function attr(node, attribute, value) {
        if (value == null)
            node.removeAttribute(attribute);
        else if (node.getAttribute(attribute) !== value)
            node.setAttribute(attribute, value);
    }
    function children(element) {
        return Array.from(element.childNodes);
    }
    function set_input_value(input, value) {
        input.value = value == null ? '' : value;
    }
    function set_style(node, key, value, important) {
        node.style.setProperty(key, value, important ? 'important' : '');
    }
    function custom_event(type, detail) {
        const e = document.createEvent('CustomEvent');
        e.initCustomEvent(type, false, false, detail);
        return e;
    }
    class HtmlTag {
        constructor(anchor = null) {
            this.a = anchor;
            this.e = this.n = null;
        }
        m(html, target, anchor = null) {
            if (!this.e) {
                this.e = element(target.nodeName);
                this.t = target;
                this.h(html);
            }
            this.i(anchor);
        }
        h(html) {
            this.e.innerHTML = html;
            this.n = Array.from(this.e.childNodes);
        }
        i(anchor) {
            for (let i = 0; i < this.n.length; i += 1) {
                insert(this.t, this.n[i], anchor);
            }
        }
        p(html) {
            this.d();
            this.h(html);
            this.i(this.a);
        }
        d() {
            this.n.forEach(detach);
        }
    }

    let current_component;
    function set_current_component(component) {
        current_component = component;
    }

    const dirty_components = [];
    const binding_callbacks = [];
    const render_callbacks = [];
    const flush_callbacks = [];
    const resolved_promise = Promise.resolve();
    let update_scheduled = false;
    function schedule_update() {
        if (!update_scheduled) {
            update_scheduled = true;
            resolved_promise.then(flush);
        }
    }
    function add_render_callback(fn) {
        render_callbacks.push(fn);
    }
    let flushing = false;
    const seen_callbacks = new Set();
    function flush() {
        if (flushing)
            return;
        flushing = true;
        do {
            // first, call beforeUpdate functions
            // and update components
            for (let i = 0; i < dirty_components.length; i += 1) {
                const component = dirty_components[i];
                set_current_component(component);
                update(component.$$);
            }
            set_current_component(null);
            dirty_components.length = 0;
            while (binding_callbacks.length)
                binding_callbacks.pop()();
            // then, once components are updated, call
            // afterUpdate functions. This may cause
            // subsequent updates...
            for (let i = 0; i < render_callbacks.length; i += 1) {
                const callback = render_callbacks[i];
                if (!seen_callbacks.has(callback)) {
                    // ...so guard against infinite loops
                    seen_callbacks.add(callback);
                    callback();
                }
            }
            render_callbacks.length = 0;
        } while (dirty_components.length);
        while (flush_callbacks.length) {
            flush_callbacks.pop()();
        }
        update_scheduled = false;
        flushing = false;
        seen_callbacks.clear();
    }
    function update($$) {
        if ($$.fragment !== null) {
            $$.update();
            run_all($$.before_update);
            const dirty = $$.dirty;
            $$.dirty = [-1];
            $$.fragment && $$.fragment.p($$.ctx, dirty);
            $$.after_update.forEach(add_render_callback);
        }
    }
    const outroing = new Set();
    function transition_in(block, local) {
        if (block && block.i) {
            outroing.delete(block);
            block.i(local);
        }
    }

    const globals = (typeof window !== 'undefined'
        ? window
        : typeof globalThis !== 'undefined'
            ? globalThis
            : global);
    function mount_component(component, target, anchor, customElement) {
        const { fragment, on_mount, on_destroy, after_update } = component.$$;
        fragment && fragment.m(target, anchor);
        if (!customElement) {
            // onMount happens before the initial afterUpdate
            add_render_callback(() => {
                const new_on_destroy = on_mount.map(run).filter(is_function);
                if (on_destroy) {
                    on_destroy.push(...new_on_destroy);
                }
                else {
                    // Edge case - component was destroyed immediately,
                    // most likely as a result of a binding initialising
                    run_all(new_on_destroy);
                }
                component.$$.on_mount = [];
            });
        }
        after_update.forEach(add_render_callback);
    }
    function destroy_component(component, detaching) {
        const $$ = component.$$;
        if ($$.fragment !== null) {
            run_all($$.on_destroy);
            $$.fragment && $$.fragment.d(detaching);
            // TODO null out other refs, including component.$$ (but need to
            // preserve final state?)
            $$.on_destroy = $$.fragment = null;
            $$.ctx = [];
        }
    }
    function make_dirty(component, i) {
        if (component.$$.dirty[0] === -1) {
            dirty_components.push(component);
            schedule_update();
            component.$$.dirty.fill(0);
        }
        component.$$.dirty[(i / 31) | 0] |= (1 << (i % 31));
    }
    function init(component, options, instance, create_fragment, not_equal, props, dirty = [-1]) {
        const parent_component = current_component;
        set_current_component(component);
        const $$ = component.$$ = {
            fragment: null,
            ctx: null,
            // state
            props,
            update: noop,
            not_equal,
            bound: blank_object(),
            // lifecycle
            on_mount: [],
            on_destroy: [],
            on_disconnect: [],
            before_update: [],
            after_update: [],
            context: new Map(parent_component ? parent_component.$$.context : options.context || []),
            // everything else
            callbacks: blank_object(),
            dirty,
            skip_bound: false
        };
        let ready = false;
        $$.ctx = instance
            ? instance(component, options.props || {}, (i, ret, ...rest) => {
                const value = rest.length ? rest[0] : ret;
                if ($$.ctx && not_equal($$.ctx[i], $$.ctx[i] = value)) {
                    if (!$$.skip_bound && $$.bound[i])
                        $$.bound[i](value);
                    if (ready)
                        make_dirty(component, i);
                }
                return ret;
            })
            : [];
        $$.update();
        ready = true;
        run_all($$.before_update);
        // `false` as a special case of no DOM component
        $$.fragment = create_fragment ? create_fragment($$.ctx) : false;
        if (options.target) {
            if (options.hydrate) {
                const nodes = children(options.target);
                // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
                $$.fragment && $$.fragment.l(nodes);
                nodes.forEach(detach);
            }
            else {
                // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
                $$.fragment && $$.fragment.c();
            }
            if (options.intro)
                transition_in(component.$$.fragment);
            mount_component(component, options.target, options.anchor, options.customElement);
            flush();
        }
        set_current_component(parent_component);
    }
    /**
     * Base class for Svelte components. Used when dev=false.
     */
    class SvelteComponent {
        $destroy() {
            destroy_component(this, 1);
            this.$destroy = noop;
        }
        $on(type, callback) {
            const callbacks = (this.$$.callbacks[type] || (this.$$.callbacks[type] = []));
            callbacks.push(callback);
            return () => {
                const index = callbacks.indexOf(callback);
                if (index !== -1)
                    callbacks.splice(index, 1);
            };
        }
        $set($$props) {
            if (this.$$set && !is_empty($$props)) {
                this.$$.skip_bound = true;
                this.$$set($$props);
                this.$$.skip_bound = false;
            }
        }
    }

    function dispatch_dev(type, detail) {
        document.dispatchEvent(custom_event(type, Object.assign({ version: '3.38.2' }, detail)));
    }
    function append_dev(target, node) {
        dispatch_dev('SvelteDOMInsert', { target, node });
        append(target, node);
    }
    function insert_dev(target, node, anchor) {
        dispatch_dev('SvelteDOMInsert', { target, node, anchor });
        insert(target, node, anchor);
    }
    function detach_dev(node) {
        dispatch_dev('SvelteDOMRemove', { node });
        detach(node);
    }
    function listen_dev(node, event, handler, options, has_prevent_default, has_stop_propagation) {
        const modifiers = options === true ? ['capture'] : options ? Array.from(Object.keys(options)) : [];
        if (has_prevent_default)
            modifiers.push('preventDefault');
        if (has_stop_propagation)
            modifiers.push('stopPropagation');
        dispatch_dev('SvelteDOMAddEventListener', { node, event, handler, modifiers });
        const dispose = listen(node, event, handler, options);
        return () => {
            dispatch_dev('SvelteDOMRemoveEventListener', { node, event, handler, modifiers });
            dispose();
        };
    }
    function attr_dev(node, attribute, value) {
        attr(node, attribute, value);
        if (value == null)
            dispatch_dev('SvelteDOMRemoveAttribute', { node, attribute });
        else
            dispatch_dev('SvelteDOMSetAttribute', { node, attribute, value });
    }
    function set_data_dev(text, data) {
        data = '' + data;
        if (text.wholeText === data)
            return;
        dispatch_dev('SvelteDOMSetData', { node: text, data });
        text.data = data;
    }
    function validate_each_argument(arg) {
        if (typeof arg !== 'string' && !(arg && typeof arg === 'object' && 'length' in arg)) {
            let msg = '{#each} only iterates over array-like objects.';
            if (typeof Symbol === 'function' && arg && Symbol.iterator in arg) {
                msg += ' You can use a spread to convert this iterable into an array.';
            }
            throw new Error(msg);
        }
    }
    function validate_slots(name, slot, keys) {
        for (const slot_key of Object.keys(slot)) {
            if (!~keys.indexOf(slot_key)) {
                console.warn(`<${name}> received an unexpected slot "${slot_key}".`);
            }
        }
    }
    /**
     * Base class for Svelte components with some minor dev-enhancements. Used when dev=true.
     */
    class SvelteComponentDev extends SvelteComponent {
        constructor(options) {
            if (!options || (!options.target && !options.$$inline)) {
                throw new Error("'target' is a required option");
            }
            super();
        }
        $destroy() {
            super.$destroy();
            this.$destroy = () => {
                console.warn('Component was already destroyed'); // eslint-disable-line no-console
            };
        }
        $capture_state() { }
        $inject_state() { }
    }

    /* src/App.svelte generated by Svelte v3.38.2 */

    const { Error: Error_1, Object: Object_1, console: console_1 } = globals;
    const file = "src/App.svelte";

    function get_each_context(ctx, list, i) {
    	const child_ctx = ctx.slice();
    	child_ctx[15] = list[i];
    	return child_ctx;
    }

    function get_each_context_1(ctx, list, i) {
    	const child_ctx = ctx.slice();
    	child_ctx[18] = list[i];
    	return child_ctx;
    }

    // (241:0) {#each 显示的指令历史 as 指令}
    function create_each_block_1(ctx) {
    	let t0;
    	let div0;
    	let t1_value = /*指令*/ ctx[18].当前路径.map(func).join("/") + "";
    	let t1;
    	let t2;
    	let div1;
    	let t3_value = /*指令*/ ctx[18].目前指令 + "";
    	let t3;
    	let t4;
    	let div2;
    	let raw_value = /*指令*/ ctx[18].指令结果 + "";

    	const block = {
    		c: function create() {
    			t0 = text("----------------------------------------------------\n  ");
    			div0 = element("div");
    			t1 = text(t1_value);
    			t2 = space();
    			div1 = element("div");
    			t3 = text(t3_value);
    			t4 = space();
    			div2 = element("div");
    			add_location(div0, file, 242, 2, 6482);
    			add_location(div1, file, 245, 2, 6542);
    			add_location(div2, file, 246, 2, 6565);
    		},
    		m: function mount(target, anchor) {
    			insert_dev(target, t0, anchor);
    			insert_dev(target, div0, anchor);
    			append_dev(div0, t1);
    			insert_dev(target, t2, anchor);
    			insert_dev(target, div1, anchor);
    			append_dev(div1, t3);
    			insert_dev(target, t4, anchor);
    			insert_dev(target, div2, anchor);
    			div2.innerHTML = raw_value;
    		},
    		p: function update(ctx, dirty) {
    			if (dirty & /*显示的指令历史*/ 2 && t1_value !== (t1_value = /*指令*/ ctx[18].当前路径.map(func).join("/") + "")) set_data_dev(t1, t1_value);
    			if (dirty & /*显示的指令历史*/ 2 && t3_value !== (t3_value = /*指令*/ ctx[18].目前指令 + "")) set_data_dev(t3, t3_value);
    			if (dirty & /*显示的指令历史*/ 2 && raw_value !== (raw_value = /*指令*/ ctx[18].指令结果 + "")) div2.innerHTML = raw_value;		},
    		d: function destroy(detaching) {
    			if (detaching) detach_dev(t0);
    			if (detaching) detach_dev(div0);
    			if (detaching) detach_dev(t2);
    			if (detaching) detach_dev(div1);
    			if (detaching) detach_dev(t4);
    			if (detaching) detach_dev(div2);
    		}
    	};

    	dispatch_dev("SvelteRegisterBlock", {
    		block,
    		id: create_each_block_1.name,
    		type: "each",
    		source: "(241:0) {#each 显示的指令历史 as 指令}",
    		ctx
    	});

    	return block;
    }

    // (255:0) {#each 显示的指令历史 as i}
    function create_each_block(ctx) {
    	let div;
    	let t_value = JSON.stringify(/*i*/ ctx[15]) + "";
    	let t;

    	const block = {
    		c: function create() {
    			div = element("div");
    			t = text(t_value);
    			add_location(div, file, 255, 2, 6708);
    		},
    		m: function mount(target, anchor) {
    			insert_dev(target, div, anchor);
    			append_dev(div, t);
    		},
    		p: function update(ctx, dirty) {
    			if (dirty & /*显示的指令历史*/ 2 && t_value !== (t_value = JSON.stringify(/*i*/ ctx[15]) + "")) set_data_dev(t, t_value);
    		},
    		d: function destroy(detaching) {
    			if (detaching) detach_dev(div);
    		}
    	};

    	dispatch_dev("SvelteRegisterBlock", {
    		block,
    		id: create_each_block.name,
    		type: "each",
    		source: "(255:0) {#each 显示的指令历史 as i}",
    		ctx
    	});

    	return block;
    }

    function create_fragment(ctx) {
    	let a;
    	let t1;
    	let div0;
    	let textarea;
    	let t2;
    	let t3;
    	let div1;
    	let input;
    	let t4;
    	let html_tag;
    	let t5;
    	let each1_anchor;
    	let mounted;
    	let dispose;
    	let each_value_1 = /*显示的指令历史*/ ctx[1];
    	validate_each_argument(each_value_1);
    	let each_blocks_1 = [];

    	for (let i = 0; i < each_value_1.length; i += 1) {
    		each_blocks_1[i] = create_each_block_1(get_each_context_1(ctx, each_value_1, i));
    	}

    	let each_value = /*显示的指令历史*/ ctx[1];
    	validate_each_argument(each_value);
    	let each_blocks = [];

    	for (let i = 0; i < each_value.length; i += 1) {
    		each_blocks[i] = create_each_block(get_each_context(ctx, each_value, i));
    	}

    	const block = {
    		c: function create() {
    			a = element("a");
    			a.textContent = "帮助";
    			t1 = space();
    			div0 = element("div");
    			textarea = element("textarea");
    			t2 = space();

    			for (let i = 0; i < each_blocks_1.length; i += 1) {
    				each_blocks_1[i].c();
    			}

    			t3 = space();
    			div1 = element("div");
    			input = element("input");
    			t4 = space();
    			t5 = space();

    			for (let i = 0; i < each_blocks.length; i += 1) {
    				each_blocks[i].c();
    			}

    			each1_anchor = empty();
    			attr_dev(a, "target", "_blank");
    			attr_dev(a, "href", "https://github.com/wuyudi/OperatingSystem/tree/online-demo#%E4%B8%80%E4%B8%AA-shell-%E7%9A%84%E5%9C%A8%E7%BA%BF%E6%A8%A1%E6%8B%9F%E5%99%A8");
    			add_location(a, file, 229, 0, 6087);
    			set_style(textarea, "font-family", "Consolas");
    			set_style(textarea, "height", "10rem");
    			set_style(textarea, "width", "80rem");
    			set_style(textarea, "border-color", "transparent");
    			textarea.value = /*logo*/ ctx[3];
    			add_location(textarea, file, 235, 2, 6275);
    			add_location(div0, file, 234, 0, 6267);
    			set_style(input, "width", "100%");
    			add_location(input, file, 251, 2, 6616);
    			html_tag = new HtmlTag(null);
    			add_location(div1, file, 250, 0, 6608);
    		},
    		l: function claim(nodes) {
    			throw new Error_1("options.hydrate only works if the component was compiled with the `hydratable: true` option");
    		},
    		m: function mount(target, anchor) {
    			insert_dev(target, a, anchor);
    			insert_dev(target, t1, anchor);
    			insert_dev(target, div0, anchor);
    			append_dev(div0, textarea);
    			insert_dev(target, t2, anchor);

    			for (let i = 0; i < each_blocks_1.length; i += 1) {
    				each_blocks_1[i].m(target, anchor);
    			}

    			insert_dev(target, t3, anchor);
    			insert_dev(target, div1, anchor);
    			append_dev(div1, input);
    			set_input_value(input, /*目前指令*/ ctx[0]);
    			append_dev(div1, t4);
    			html_tag.m(/*指令结果*/ ctx[2], div1);
    			insert_dev(target, t5, anchor);

    			for (let i = 0; i < each_blocks.length; i += 1) {
    				each_blocks[i].m(target, anchor);
    			}

    			insert_dev(target, each1_anchor, anchor);

    			if (!mounted) {
    				dispose = [
    					listen_dev(window, "keydown", /*按键按下*/ ctx[4], false, false, false),
    					listen_dev(input, "input", /*input_input_handler*/ ctx[6])
    				];

    				mounted = true;
    			}
    		},
    		p: function update(ctx, [dirty]) {
    			if (dirty & /*显示的指令历史*/ 2) {
    				each_value_1 = /*显示的指令历史*/ ctx[1];
    				validate_each_argument(each_value_1);
    				let i;

    				for (i = 0; i < each_value_1.length; i += 1) {
    					const child_ctx = get_each_context_1(ctx, each_value_1, i);

    					if (each_blocks_1[i]) {
    						each_blocks_1[i].p(child_ctx, dirty);
    					} else {
    						each_blocks_1[i] = create_each_block_1(child_ctx);
    						each_blocks_1[i].c();
    						each_blocks_1[i].m(t3.parentNode, t3);
    					}
    				}

    				for (; i < each_blocks_1.length; i += 1) {
    					each_blocks_1[i].d(1);
    				}

    				each_blocks_1.length = each_value_1.length;
    			}

    			if (dirty & /*目前指令*/ 1 && input.value !== /*目前指令*/ ctx[0]) {
    				set_input_value(input, /*目前指令*/ ctx[0]);
    			}

    			if (dirty & /*指令结果*/ 4) html_tag.p(/*指令结果*/ ctx[2]);

    			if (dirty & /*JSON, 显示的指令历史*/ 2) {
    				each_value = /*显示的指令历史*/ ctx[1];
    				validate_each_argument(each_value);
    				let i;

    				for (i = 0; i < each_value.length; i += 1) {
    					const child_ctx = get_each_context(ctx, each_value, i);

    					if (each_blocks[i]) {
    						each_blocks[i].p(child_ctx, dirty);
    					} else {
    						each_blocks[i] = create_each_block(child_ctx);
    						each_blocks[i].c();
    						each_blocks[i].m(each1_anchor.parentNode, each1_anchor);
    					}
    				}

    				for (; i < each_blocks.length; i += 1) {
    					each_blocks[i].d(1);
    				}

    				each_blocks.length = each_value.length;
    			}
    		},
    		i: noop,
    		o: noop,
    		d: function destroy(detaching) {
    			if (detaching) detach_dev(a);
    			if (detaching) detach_dev(t1);
    			if (detaching) detach_dev(div0);
    			if (detaching) detach_dev(t2);
    			destroy_each(each_blocks_1, detaching);
    			if (detaching) detach_dev(t3);
    			if (detaching) detach_dev(div1);
    			if (detaching) detach_dev(t5);
    			destroy_each(each_blocks, detaching);
    			if (detaching) detach_dev(each1_anchor);
    			mounted = false;
    			run_all(dispose);
    		}
    	};

    	dispatch_dev("SvelteRegisterBlock", {
    		block,
    		id: create_fragment.name,
    		type: "component",
    		source: "",
    		ctx
    	});

    	return block;
    }

    const func = x => x.name;

    function instance($$self, $$props, $$invalidate) {
    	let { $$slots: slots = {}, $$scope } = $$props;
    	validate_slots("App", slots, []);

    	class FileNode {
    		constructor(name, content) {
    			this.name = name;
    			this.content = content !== null && content !== void 0 ? content : "";
    		}
    	}

    	class TreeNode {
    		constructor(name) {
    			this.name = name;
    			this.dir_children = {};
    			this.file_children = {};
    		}
    	}

    	let logo = String.raw`
             _ _                      _          _ _ 
  ___  _ __ | (_)_ __   ___       ___| |__   ___| | |
 / _ \| '_ \| | | '_ \ / _ \_____/ __| '_ \ / _ \ | |
| (_) | | | | | | | | |  __/_____\__ \ | | |  __/ | |
 \___/|_| |_|_|_|_| |_|\___|     |___/_| |_|\___|_|_|
`;

    	let 文件树 = new TreeNode("~");
    	let 路径历史 = [文件树];
    	const 目前位置 = () => 路径历史[路径历史.length - 1];

    	let 走向路径 = (目前地址, 文件夹序列) => {
    		let temp = 路径历史.slice();

    		while (文件夹序列.length > 0) {
    			console.log(`文件夹序列 = ${文件夹序列}`);
    			let next = 文件夹序列.shift();

    			if (next !== "..") {
    				目前地址 = 目前地址.dir_children[next];

    				if (typeof 目前地址 === "undefined") {
    					console.log("can't get to");
    					throw new Error("can't get to");
    				} else {
    					temp.push(目前地址);
    				}
    			} else {
    				if (temp.length > 1) {
    					temp.pop();
    					目前地址 = temp[temp.length - 1];
    				}
    			}
    		}

    		return temp;
    	};

    	const cd = path => {
    		// 检查是否是从根目录或者同一文件夹
    		// 最外层修改全局变量
    		// 中间层测试能否返回
    		// 内层测试每一层是否能到达
    		let 文件夹序列 = path.split("/");

    		console.log(文件夹序列);

    		try {
    			$$invalidate(5, 路径历史 = 走向路径(路径历史[路径历史.length - 1], 文件夹序列));
    			return "";
    		} catch(_a) {
    			return "无此目录";
    		}
    	};

    	let 指令映射 = new Map([
    			[
    				"ls",
    				args => {
    					return Object.keys(目前位置().dir_children).map(child => `<div> ${child}  文件夹</div>`).join(" ") + Object.keys(目前位置().file_children).map(child => `<div> ${child}  文件</div>`).join(" ");
    				}
    			],
    			[
    				"mkdir",
    				name => {
    					name.map(name => 目前位置().dir_children[name] = new TreeNode(name));
    					return "";
    				}
    			],
    			[
    				"cd",
    				_path => {
    					let path = _path[0];

    					if ([...path].every(x => x === ".")) {
    						let l = path.length;

    						while (l > 1 && 路径历史.length > 1) {
    							路径历史.pop();
    							l--;
    						}

    						return "";
    					} else if (!path.includes("/")) {
    						let 下一站 = 目前位置().dir_children[path];

    						if (typeof 下一站 !== "undefined") {
    							路径历史.push(下一站);
    							console.log("新建成功");
    							return "";
    						} else {
    							return "can't cd to path";
    						}
    					} else {
    						console.log(path);
    						cd(path);
    						return "";
    					}
    				}
    			],
    			[
    				"touch",
    				args => {
    					args.map(arg => 目前位置().file_children[arg] = new FileNode(arg));
    					return "创建完成";
    				}
    			],
    			[
    				"cat",
    				args => {
    					let arg = args[0];

    					if (目前位置().file_children.hasOwnProperty(arg)) {
    						return 目前位置().file_children[arg].content;
    					} else {
    						return `cat: ${arg}: 没有那个文件`;
    					}
    				}
    			],
    			[
    				"echo",
    				/**
     * @description 接受形如 `echo aaa > file`
     * 和 `echo aaa >> file`
     */
    				args => {
    					let 写入内容 = args[0];
    					let 目标文件 = args[2];
    					let 文件 = 目前位置().file_children[目标文件];

    					if (目前位置().file_children.hasOwnProperty(目标文件)) {
    						目前位置().file_children[目标文件] = new FileNode(目标文件, 写入内容);
    						return "";
    					}

    					if (args[1] === ">") {
    						文件.content = 写入内容;
    						return "写入成功";
    					}

    					if (args[1] === ">>") {
    						文件.content += 写入内容;
    						return "追加成功";
    					}
    				}
    			],
    			[
    				"rmdir",
    				args => {
    					[...new Set(args)].map(arg => delete 目前位置().dir_children[arg]);
    					return "删除成功";
    				}
    			],
    			[
    				"rm",
    				args => {
    					[...new Set(args)].map(arg => delete 目前位置().file_children[arg]);
    					return "删除成功";
    				}
    			],
    			[
    				"cp",
    				args => {
    					let from = args[0].split("/");
    					let 源文件名 = from[from.length - 1];
    					let to = args[1].split("/");
    					let 终点文件名 = to[to.length - 1];
    					let 起点文件夹 = 走向路径(目前位置(), from.slice(0, -1));
    					let 中转站 = 起点文件夹[起点文件夹.length - 1].file_children[源文件名];

    					if (typeof 中转站 === "undefined") {
    						return "无此文件";
    					}

    					console.log("复制成功");
    					console.log(`复制成功 后 目前位置 ${JSON.stringify(目前位置())}`);
    					let 终点文件夹 = 走向路径(目前位置(), to.slice(0, -1));
    					终点文件夹[终点文件夹.length - 1].file_children[终点文件名] = new FileNode(终点文件名, 中转站.content);
    					return "";
    				}
    			]
    		]);

    	function 计算结果(指令) {
    		let 分割指令 = 指令.split(" ");
    		let 头部 = 分割指令[0];
    		let 参数列表 = 分割指令.slice(1);
    		let 指令列表 = new Set(指令映射.keys());

    		if (指令列表.has(头部)) {
    			return 指令映射.get(头部)(参数列表);
    		} else {
    			return "error, 无此指令";
    		}
    	}

    	let 目前指令 = "";
    	let 显示的指令历史 = [{ 目前指令: "", 指令结果: "", 当前路径: [文件树] }];
    	let 指令结果 = "";

    	const 按键按下 = 事件 => {
    		if (事件.key === "Enter") {
    			if (目前指令 === "") {
    				return;
    			}

    			$$invalidate(2, 指令结果 = 计算结果(目前指令));
    			$$invalidate(1, 显示的指令历史[显示的指令历史.length - 1].目前指令 = 目前指令, 显示的指令历史);
    			$$invalidate(1, 显示的指令历史[显示的指令历史.length - 1].指令结果 = 指令结果, 显示的指令历史);
    			显示的指令历史.push({ 目前指令: "", 指令结果: "", 当前路径: 路径历史.concat() });
    			$$invalidate(0, 目前指令 = "");
    			$$invalidate(2, 指令结果 = "");
    		}

    		console.log(事件.key);
    	};

    	const writable_props = [];

    	Object_1.keys($$props).forEach(key => {
    		if (!~writable_props.indexOf(key) && key.slice(0, 2) !== "$$") console_1.warn(`<App> was created with unknown prop '${key}'`);
    	});

    	function input_input_handler() {
    		目前指令 = this.value;
    		$$invalidate(0, 目前指令);
    	}

    	$$self.$capture_state = () => ({
    		FileNode,
    		TreeNode,
    		logo,
    		文件树,
    		路径历史,
    		目前位置,
    		走向路径,
    		cd,
    		指令映射,
    		计算结果,
    		目前指令,
    		显示的指令历史,
    		指令结果,
    		按键按下
    	});

    	$$self.$inject_state = $$props => {
    		if ("logo" in $$props) $$invalidate(3, logo = $$props.logo);
    		if ("文件树" in $$props) 文件树 = $$props.文件树;
    		if ("路径历史" in $$props) $$invalidate(5, 路径历史 = $$props.路径历史);
    		if ("走向路径" in $$props) 走向路径 = $$props.走向路径;
    		if ("指令映射" in $$props) 指令映射 = $$props.指令映射;
    		if ("目前指令" in $$props) $$invalidate(0, 目前指令 = $$props.目前指令);
    		if ("显示的指令历史" in $$props) $$invalidate(1, 显示的指令历史 = $$props.显示的指令历史);
    		if ("指令结果" in $$props) $$invalidate(2, 指令结果 = $$props.指令结果);
    	};

    	if ($$props && "$$inject" in $$props) {
    		$$self.$inject_state($$props.$$inject);
    	}

    	$$self.$$.update = () => {
    		if ($$self.$$.dirty & /*路径历史*/ 32) {
    			{
    				$$invalidate(5, 路径历史);
    				console.log(路径历史);
    			}
    		}
    	};

    	return [目前指令, 显示的指令历史, 指令结果, logo, 按键按下, 路径历史, input_input_handler];
    }

    class App extends SvelteComponentDev {
    	constructor(options) {
    		super(options);
    		init(this, options, instance, create_fragment, safe_not_equal, {});

    		dispatch_dev("SvelteRegisterComponent", {
    			component: this,
    			tagName: "App",
    			options,
    			id: create_fragment.name
    		});
    	}
    }

    const app = new App({
        target: document.body,
    });

    return app;

}());
//# sourceMappingURL=bundle.js.map
