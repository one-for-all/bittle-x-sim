declare module "*.ino" {
  const content: string;
  export default content;
}

declare module "*.h" {
  const content: string;
  export default content;
}

declare module "*.md" {
  const content: string;
  export default content;
}

declare module "*.hex" {
  const content: string;
  export default content;
}

declare module "*.bin" {
  const content: ArrayBuffer;
  export default content;
}

declare module "*.txt" {
  const content: string;
  export default content;
}
