;
;  68HC11 Port Equivalency:
;

; Offset Pointers to Regs:
        .globl  pporta,ppioc,pportc
        .globl  pportb,pportcl,pddrc
        .globl  pportd,pddrd,pporte,pcforc
        .globl  poc1m,poc1d,ptcnt,ptcnth,ptcntl
        .globl  ptic1,ptic1h,ptic1l,ptic2,ptic2h,ptic2l
		.globl	ptic3,ptic3h,ptic3l,ptoc1,ptoc1h,ptoc1l
        .globl  ptoc2,ptoc2h,ptoc2l,ptoc3,ptoc3h,ptoc3l
		.globl	ptoc4,ptoc4h,ptoc4l,pti4o5,pti4o5h,pti4o5l
        .globl  ptctl1,ptctl2,ptmsk1,ptflg1
        .globl  ptmsk2,ptflg2,ppactl,ppacnt
        .globl  pspcr,pspsr,pspdr,pbaud
        .globl  psccr1,psccr2,pscsr,pscdr
        .globl  padctl,padr1,padr2,padr3
        .globl  padr4,pbprot,peprog
        .globl  poption,pcoprst,ppprog
        .globl  phprio,pinit,ptest1,pconfig

; Direct Pointers to Regs:
        .globl  porta,pioc,portc
        .globl  portb,portcl,ddrc
        .globl  portd,ddrd,porte,cforc
        .globl  oc1m,oc1d,tcnt,tcnth,tcntl
        .globl  tic1,tic1h,tic1l,tic2,tic2h,tic2l
		.globl	tic3,tic3h,tic3l,toc1,toc1h,toc1l
        .globl  toc2,toc2h,toc2l,toc3,toc3h,toc3l
		.globl	toc4,toc4h,toc4l,ti4o5,ti4o5h,ti4o5l
        .globl  tctl1,tctl2,tmsk1,tflg1
        .globl  tmsk2,tflg2,pactl,pacnt
        .globl  spcr,spsr,spdr,baud
        .globl  sccr1,sccr2,scsr,scdr
        .globl  adctl,adr1,adr2,adr3
        .globl  adr4,bprot,eprog
        .globl  option,coprst,pprog
        .globl  hprio,init,test1,config
