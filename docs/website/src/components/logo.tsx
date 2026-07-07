// components/logo.tsx
import Image from 'next/image';
import ClotIcon from '../../public/clot-nb.png';
import ClotIconDark from '../../public/clot-white-glyph.png';

export function Logo() {
  return (
    <div className="flex items-center gap-2.5">
      <span className="relative block size-[30px]">
        {/* Light mode: original logo (the </> cutout shows the light page → reads white) */}
        <Image
          src={ClotIcon}
          alt="ClotLang Logo"
          width={30}
          height={30}
          className="rounded-sm block dark:hidden"
          priority
        />
        {/* Dark mode: the </> cutout is filled white so it stays visible on dark backgrounds */}
        <Image
          src={ClotIconDark}
          alt="ClotLang Logo"
          width={30}
          height={30}
          className="rounded-sm hidden dark:block"
          priority
        />
      </span>
      <span className="font-bold text-lg max-md:hidden">
        ClotLang
      </span>
    </div>
  );
}
