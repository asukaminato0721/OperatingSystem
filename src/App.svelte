<script lang="ts">
  import { writable } from "svelte/store";
  enum Type {
    File,
    Folder,
  }
  class Node {
    name: string;
    content?: string;
    type: Type;
    children?: {
      [key: string]: Node;
    };
    constructor(name: string, type: Type, content?: string) {
      this.name = name;
      this.type = type;
      this.content = content ?? "";
      this.children = {};
    }
  }
  function last<T>(list: T[]): T {
    return list[list.length - 1];
  }
  let logo = String.raw`
             _ _                      _          _ _ 
  ___  _ __ | (_)_ __   ___       ___| |__   ___| | |
 / _ \| '_ \| | | '_ \ / _ \_____/ __| '_ \ / _ \ | |
| (_) | | | | | | | | |  __/_____\__ \ | | |  __/ | |
 \___/|_| |_|_|_|_| |_|\___|     |___/_| |_|\___|_|_|
`;
  let 文件树 =
    JSON.parse(localStorage.getItem("在线终端")) ?? new Node("~", Type.Folder);
  let 路径历史: Node[] = [文件树];
  const 目前位置 = () => last(路径历史);
  let 走向路径 = (目前地址: Node, 文件夹序列: string[]) => {
    let temp = 路径历史.slice();
    while (文件夹序列.length > 0) {
      console.log(`文件夹序列 = ${文件夹序列}`);
      let next = 文件夹序列.shift();
      if (next !== "..") {
        目前地址 = 目前地址.children[next];
        if (typeof 目前地址 === "undefined" || 目前地址.type === Type.File) {
          console.log("can't get to");
          throw new Error("can't get to");
        } else {
          temp.push(目前地址);
        }
      } else {
        if (temp.length > 1) {
          temp.pop();
          目前地址 = last(temp);
        }
      }
    }
    return temp;
  };
  const cd = (path: string) => {
    // 检查是否是从根目录或者同一文件夹
    // 最外层修改全局变量
    // 中间层测试能否返回
    // 内层测试每一层是否能到达
    let 文件夹序列 = path.split("/");
    console.log(文件夹序列);
    try {
      路径历史 = 走向路径(last(路径历史), 文件夹序列);
      return "";
    } catch {
      return "无此目录";
    }
  };
  let 指令映射 = new Map([
    [
      "ls",
      (args: string[]) => {
        return Object.values(目前位置().children)
          .map(
            (child) =>
              `<div> ${child.name}  ${
                child.type === Type.Folder ? `文件夹` : `文件`
              }</div>`
          )
          .join(" ");
      },
    ],
    [
      "mkdir",
      (names: string[]) => {
        const 返回: string[] = [];
        const 已存在 = new Set(Object.keys(目前位置().children));
        names.map((name) => {
          if (已存在.has(name)) {
            返回.push(`mkdir: 无法创建目录 “${name}”: 文件已存在`);
          } else {
            目前位置().children[name] = new Node(name, Type.Folder);
          }
        });
        return 返回.join("</br>");
      },
    ],
    [
      "cd",
      (_path: string[]) => {
        let path = _path[0];
        if ([...path].every((x) => x === ".")) {
          let l = path.length;
          while (l > 1 && 路径历史.length > 1) {
            路径历史.pop();
            l--;
          }
          return "";
        } else if (!path.includes("/")) {
          let 下一站 = 目前位置().children[path];
          if (typeof 下一站 !== "undefined" && 下一站.type === Type.Folder) {
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
      },
    ],
    [
      "touch",
      (names: string[]) => {
        const 返回: string[] = [];
        const 已存在 = new Set(Object.keys(目前位置().children));
        names.map((name) => {
          if (已存在.has(name)) {
            返回.push(`touch: 无法创建目录 “${name}”: 文件已存在`);
          } else {
            目前位置().children[name] = new Node(name, Type.File);
          }
        });
        return 返回.join("</br>");
      },
    ],
    [
      "cat",
      (args: string[]) => {
        let arg = args[0];
        let 临门一脚: {
          [key: string]: Node;
        };
        let 要显示的文件名: string;
        if (!arg.includes("/")) {
          临门一脚 = 目前位置().children;
          要显示的文件名 = arg;
        } else {
          const 路径 = arg.split("/");
          临门一脚 = last(走向路径(目前位置(), 路径.slice(0, -1))).children;
          console.log(JSON.stringify(临门一脚));
          要显示的文件名 = last(路径);
        }
        if (
          临门一脚.hasOwnProperty(要显示的文件名) &&
          临门一脚[要显示的文件名].type === Type.File
        ) {
          return 临门一脚[要显示的文件名].content;
        } else {
          return `cat: ${arg}: 没有那个文件`;
        }
      },
    ],
    [
      "echo",
      /**
       * @description 接受形如 `echo aaa > file`
       * 和 `echo aaa >> file`
       */
      (args: string[]) => {
        /*
        1. 存在，是 folder
        2. 存在，是 file
        3. 不存在
        */
        let 写入内容 = args[0];
        let 目标文件 = args[2];
        let 文件 = 目前位置().children[目标文件];
        if (
          目前位置().children.hasOwnProperty(目标文件) &&
          文件.type === Type.Folder
        ) {
          return `是一个目录: ${目标文件}`;
        }
        if (!目前位置().children.hasOwnProperty(目标文件)) {
          目前位置().children[目标文件] = new Node(
            目标文件,
            Type.File,
            写入内容
          );
          return "写入成功";
        } else {
          if (args[1] === ">") {
            文件.content = 写入内容;
            return "写入成功";
          }
          if (args[1] === ">>") {
            文件.content += 写入内容;
            return "追加成功";
          }
        }
      },
    ],
    [
      "rm",
      (args: string[]) => {
        const 返回值: string[] = [];
        const 当前有的 = new Set(Object.keys(目前位置().children));
        [...new Set(args)].map((arg) => {
          if (当前有的.has(arg)) {
            delete 目前位置().children[arg];
          } else {
            返回值.push(`rm: 无法删除 '${arg}': 没有那个文件或目录`);
          }
        });
        return 返回值.join(`</br>`);
      },
    ],
    [
      "cp",
      (args: string[]) => {
        let from = args[0].split("/");
        let 源文件名 = last(from);
        let to = args[1].split("/");
        let 终点文件名 = last(to);
        let 起点文件夹 = 走向路径(目前位置(), from.slice(0, -1));
        let 中转站 = last(起点文件夹).children[源文件名];
        if (typeof 中转站 === "undefined") {
          return "无此文件";
        }
        console.log("复制成功");
        console.log(`复制成功 后 目前位置 ${JSON.stringify(目前位置())}`);
        let 终点文件夹 = 走向路径(目前位置(), to.slice(0, -1));
        last(终点文件夹).children[终点文件名] = 中转站;
        last(终点文件夹).children[终点文件名].name = 终点文件名;
        return "";
      },
    ],
    [
      "clear",
      (args: string[]) => {
        显示的指令历史 = [
          {
            目前指令: "",
            指令结果: "",
            当前路径: last(显示的指令历史).当前路径,
          },
        ];
        return "";
      },
    ],
    [
      "exit",
      (args: string[]) => {
        window.close();
        return "";
      },
    ],
    [
      "tree",
      (args: string[]) => {
        function dfs(root: Node, 缩进: string = `&nbsp;&nbsp;&nbsp;&nbsp;`) {
          if (root.type === Type.File) {
            return `<div>${缩进} ${root.name} 文件</div>`;
          }
          return `<div>${缩进} ${root.name} 文件夹</div>
          <div> ${Object.values(root.children)
            .map((x) => dfs(x, 缩进 + `&nbsp;&nbsp;&nbsp;&nbsp;`))
            .join("<br/>")} 
          </div>`;
        }
        return dfs(目前位置(), `<br/>`);
      },
    ],
  ]);
  function 计算结果(指令: string): string {
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
  let 显示的指令历史: {
    目前指令: string;
    指令结果: string;
    当前路径: Node[];
  }[] = [{ 目前指令: "", 指令结果: "", 当前路径: [文件树] }];
  let 指令结果 = "";
  const 按键按下 = (事件: KeyboardEvent) => {
    if (事件.key === "Enter") {
      if (目前指令 === "") {
        return;
      }
      指令结果 = 计算结果(目前指令);
      if (目前指令 !== "clear") {
        显示的指令历史[显示的指令历史.length - 1].目前指令 = 目前指令;
        显示的指令历史[显示的指令历史.length - 1].指令结果 = 指令结果;
        显示的指令历史.push({
          目前指令: "",
          指令结果: "",
          当前路径: 路径历史.concat(),
        });
      }
      目前指令 = "";
      指令结果 = "";
    }
    console.log(事件.key);
  };
</script>

<svelte:window on:keydown={按键按下} />

<a
  target="_blank"
  href="https://github.com/wuyudi/OperatingSystem/tree/online-demo#%E4%B8%80%E4%B8%AA-shell-%E7%9A%84%E5%9C%A8%E7%BA%BF%E6%A8%A1%E6%8B%9F%E5%99%A8"
  >帮助</a
>
<button
  on:click={() => localStorage.setItem("在线终端", JSON.stringify(文件树))}
>
  保存
</button>
<div>
  <textarea
    style="font-family: Consolas;height: 10rem;width: 80rem;border-color: transparent; "
    value={logo}
  />
</div>
{#each 显示的指令历史 as 指令}
  ----------------------------------------------------
  <div>
    {指令.当前路径.map((x) => x.name).join("/")}
  </div>
  <div>{指令.目前指令}</div>
  <div>
    {@html 指令.指令结果}
  </div>
{/each}
<div>
  <input bind:value={目前指令} style="width:100%" />
  {@html 指令结果}
</div>
<!-- {#each 显示的指令历史 as i}
  <div>{JSON.stringify(i)}</div>
{/each} -->
