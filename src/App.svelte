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
    children: (FileNode | TreeNode)[];
    constructor(name: string) {
      this.name = name;
      this.children = [];
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
  let 前缀;
  $: {
    路径历史 = 路径历史;
    前缀 = 路径历史.map((x) => x.name).join("/");
    console.log(路径历史);
  }
  let 指令映射 = new Map([
    [
      "ls",
      (args: string[]) => {
        return 路径历史[路径历史.length - 1].children
          .map((child) => child.name)
          .join(" ");
      },
    ],
    [
      "mkdir",
      (name: string[]) => {
        路径历史[路径历史.length - 1].children.push(new TreeNode(name[0]));
        return "";
      },
    ],
    [
      "cd",
      (_path: string[]) => {
        let path = _path[0];
        if (path === "..") {
          if (路径历史.length > 1) {
            路径历史.pop();
            return "";
          } else {
            return "can't cd to path";
          }
        }
        let 下一站 = 路径历史[路径历史.length - 1].children.find(
          (x) => x instanceof TreeNode && x.name === path
        );
        if (typeof 下一站 !== "undefined" && 下一站 instanceof TreeNode) {
          路径历史.push(下一站);
          console.log("新建成功");
          return "";
        } else {
          return "can't cd to path";
        }
      },
    ],
    [
      "touch",
      (args: string[]) => {
        路径历史[路径历史.length - 1].children.push(
          ...args.map((x) => new FileNode(x))
        );
        return "创建完成";
      },
    ],
    [
      "cat",
      (args: string[]) => {
        let arg = args[0];
        let 文件 = 路径历史[路径历史.length - 1].children.find(
          (x) => x.name === arg && x instanceof FileNode
        );
        if (typeof 文件 !== "undefined" && 文件 instanceof FileNode) {
          return 文件.content;
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
        let 文件 = 路径历史[路径历史.length - 1].children.find(
          (x) => x.name === 目标文件 && x instanceof FileNode
        );
        if (typeof 文件 === "undefined") {
          路径历史[路径历史.length - 1].children.push(
            new FileNode(目标文件, 写入内容)
          );
          return "";
        }
        if (文件 instanceof FileNode) {
          if (args[1] === ">") {
            文件.content = 写入内容;
            return "";
          }
          if (args[1] === ">>") {
            文件.content += 写入内容;
            return "";
          }
        }
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
    if (事件.key === "Tab") {
      let 单词列表 = 目前指令.split(" ");
      let 待补全单词 = 单词列表[单词列表.length - 1];
      // TODO: 补全单词
    }
    console.log(事件.key);
  };
</script>

<svelte:window on:keydown={按键按下} />
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
    {指令.指令结果}
  </div>
{/each}
<div>
  <input bind:value={目前指令} style="width:100%" />
  {指令结果}
</div>
{#each 显示的指令历史 as i}
  <div>{JSON.stringify(i)}</div>
{/each}
