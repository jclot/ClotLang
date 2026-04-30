// components/logo.tsx
import Image from 'next/image';
import ClotIcon from '../../public/clot-nb.png'; 

export function Logo() {
  return (
    <div className="flex items-center gap-2.5">
      <Image
        src={ClotIcon}
        alt="ClotLang Logo"
        width={30}
        height={30}
        className="rounded-sm"
        priority 
      />
      <span className="font-bold text-lg max-md:hidden">
        ClotLang
      </span>
    </div>
  );
}