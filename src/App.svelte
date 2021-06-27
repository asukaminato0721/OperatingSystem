<script lang="ts">
  class FileNode {
    name: string;
    content: string;
    constructor(name: string, content?: string) {
      this.name = name;
      this.content = content ?? "";
    }
  }
  class TreeNode {
    name: string;
    dir_children: {
      [key: string]: TreeNode;
    };
    file_children: {
      [key: string]: FileNode;
    };
    constructor(name: string) {
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
  let 路径历史: TreeNode[] = [文件树];
  const 目前位置 = () => 路径历史[路径历史.length - 1];
  let 走向路径 = (目前地址: TreeNode, 文件夹序列: string[]) => {
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
  const cd = (path: string) => {
    // 检查是否是从根目录或者同一文件夹
    // 最外层修改全局变量
    // 中间层测试能否返回
    // 内层测试每一层是否能到达
    let 文件夹序列 = path.split("/");
    console.log(文件夹序列);
    try {
      路径历史 = 走向路径(路径历史[路径历史.length - 1], 文件夹序列);
      return "";
    } catch {
      return "无此目录";
    }
  };
  $: {
    路径历史 = 路径历史;
    console.log(路径历史);
  }
  let 指令映射 = new Map([
    [
      "ls",
      (args: string[]) => {
        return (
          Object.keys(目前位置().dir_children)
            .map((child) => `<div> ${child}  文件夹</div>`)
            .join(" ") +
          Object.keys(目前位置().file_children)
            .map((child) => `<div> ${child}  文件</div>`)
            .join(" ")
        );
      },
    ],
    [
      "mkdir",
      (name: string[]) => {
        name.map(
          (name) => (目前位置().dir_children[name] = new TreeNode(name))
        );
        return "";
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
      },
    ],
    [
      "touch",
      (args: string[]) => {
        args.map((arg) => (目前位置().file_children[arg] = new FileNode(arg)));
        return "创建完成";
      },
    ],
    [
      "cat",
      (args: string[]) => {
        let arg = args[0];
        if (目前位置().file_children.hasOwnProperty(arg)) {
          return 目前位置().file_children[arg].content;
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
      },
    ],
    [
      "rmdir",
      (args: string[]) => {
        [...new Set(args)].map((arg) => delete 目前位置().dir_children[arg]);
        return "删除成功";
      },
    ],
    [
      "rm",
      (args: string[]) => {
        [...new Set(args)].map((arg) => delete 目前位置().file_children[arg]);
        return "删除成功";
      },
    ],
    [
      "cp",
      (args: string[]) => {
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

        终点文件夹[终点文件夹.length - 1].file_children[终点文件名] =
          new FileNode(终点文件名, 中转站.content);
        return "";
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
    当前路径: TreeNode[];
  }[] = [{ 目前指令: "", 指令结果: "", 当前路径: [文件树] }];
  let 指令结果 = "";
  const 按键按下 = (事件: KeyboardEvent) => {
    if (事件.key === "Enter") {
      if (目前指令 === "") {
        return;
      }
      指令结果 = 计算结果(目前指令);
      显示的指令历史[显示的指令历史.length - 1].目前指令 = 目前指令;
      显示的指令历史[显示的指令历史.length - 1].指令结果 = 指令结果;
      显示的指令历史.push({
        目前指令: "",
        指令结果: "",
        当前路径: 路径历史.concat(),
      });
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
{#each 显示的指令历史 as i}
  <div>{JSON.stringify(i)}</div>
{/each}
