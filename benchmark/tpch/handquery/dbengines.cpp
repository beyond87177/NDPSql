#include <chrono>



#include "dbengines.h"


#define GRPnotfound()                                           \
  do {                                                          \
    /* no equal found: start new group */                       \
    if (ngrp == maxgrps) {                                      \
      /* we need to extend extents and histo bats, */           \
      /* do it at most once */                                  \
      maxgrps = colsz;                                          \
      if (extents) {                                            \
        BATsetcount(en, ngrp);                                  \
        if (BATextend(en, maxgrps) != GDK_SUCCEED)              \
          goto error;                                           \
        exts = (oid *) Tloc(en, 0);                             \
      }                                                         \
      if (histo) {                                              \
        BATsetcount(hn, ngrp);                                  \
        if (BATextend(hn, maxgrps) != GDK_SUCCEED)              \
          goto error;                                           \
        cnts = (lng *) Tloc(hn, 0);                             \
      }                                                         \
    }                                                           \
    if (extents)                                                \
      exts[ngrp] = hseqb + p;                                   \
    if (histo)                                                  \
      cnts[ngrp] = 1;                                           \
    fprintf(stderr, "new group = %lu @ r = %lu\n", ngrp, r);    \
    ngrps[r] = ngrp++;                                          \
  } while (0)


#define GRP_create_partial_hash_table_core(INIT_1,HASH,COMP,ASSERT,GRPTST) \
  do {                                                                  \
    if (cand) {                                                         \
      fprintf(stderr, "partial_ht cnt = %lu\n",cnt);                    \
      for (r = 0; r < cnt; r++) {                                       \
        /*if (r%1000000 == 0) fprintf(stderr, "partial_ht r = %lu\n",r);*/ \
        p = cand[r];                                                    \
        assert(p < end);                                                \
        INIT_1;                                                         \
        prb = HASH;                                                     \
        for (hb = HASHget(hs, prb);                                     \
             hb != HASHnil(hs) && hb >= start;                          \
             hb = HASHgetlink(hs, hb)) {                                \
          ASSERT;                                                       \
          q = r;                                                        \
          while (q != 0 && cand[--q] > hb)                              \
            ;                                                           \
          if (cand[q] != hb)                                            \
            continue;                                                   \
          /*q = hb - start;*/                                           \
          GRPTST(q, r);                                                 \
          grp = ngrps[q];                                               \
          if (COMP) {                                                   \
            ngrps[r] = grp;                                             \
            if (histo)                                                  \
              cnts[grp]++;                                              \
            if (gn->tsorted &&                                          \
                grp != ngrp - 1)                                        \
              gn->tsorted = 0;                                          \
            break;                                                      \
          }                                                             \
        }                                                               \
        if (hb == HASHnil(hs) || hb < start) {                          \
          GRPnotfound();                                                \
          /* enter new group into hash table */                         \
          HASHputlink(hs, p, HASHget(hs, prb));                         \
          HASHput(hs, prb, p);                                          \
        }                                                               \
      }                                                                 \
    } else {                                                            \
      fprintf(stderr, "I don't think there is a candlist, cnt = %lu\n", cnt); \
      for (r = 0; r < cnt; r++) {                                       \
        p = start + r;                                                  \
        assert(p < end);                                                \
        INIT_1;                                                         \
        prb = HASH;                                                     \
        /*if ( r % 10000000 == 0) fprintf(stderr, "r = %lu, p = %lu, b[p] = %lx, grps[r] = %lx, prb = %lx\n", r, p, (unsigned long int)(bbb[p]), grps[r], prb);*/ \
        for (hb = HASHget(hs, prb);                                     \
             hb != HASHnil(hs) && hb >= start;                          \
             hb = HASHgetlink(hs, hb)) {                                \
          ASSERT;                                                       \
          GRPTST(hb - start, r);                                        \
          grp = ngrps[hb - start];                                      \
          if (COMP) {                                                   \
            ngrps[r] = grp;                                             \
            if (histo)                                                  \
              cnts[grp]++;                                              \
            if (gn->tsorted &&                                          \
                grp != ngrp - 1)                                        \
              gn->tsorted = 0;                                          \
            break;                                                      \
          }                                                             \
        }                                                               \
        if (hb == HASHnil(hs) || hb < start) {                          \
          GRPnotfound();                                                \
          /* enter new group into hash table */                         \
          HASHputlink(hs, p, HASHget(hs, prb));                         \
          HASHput(hs, prb, p);                                          \
        }                                                               \
      }                                                                 \
    }                                                                   \
  } while (0)

#define NOGRPTST(i, j)	(void) 0

size_t getFilesize(const char* filename) {
  struct stat st;
  stat(filename, &st);
  return st.st_size;
}


FRec* mapfile(const char* fname){
  FRec *frec = new FRec;
  frec->fd = open(fname, O_RDONLY, 0);
  frec->fs = getFilesize(fname);
  assert((frec->fd)!=-1);
  frec->base = mmap(NULL, frec->fs, PROT_READ, MAP_SHARED, frec->fd, 0);
  assert((frec->base)!=MAP_FAILED);
  return frec;
}

void unmapfile(FRec* frec){
  int rc = munmap(frec->base, frec->fs);
  assert(rc==0);
  close(frec->fd);
}

template<typename T>
BAT* select(T* column, size_t count, T lv, T hv){
  // std::vector<size_t>* localv = new std::vector<size_t>[omp_get_max_threads()];
  // std::vector<size_t> localcnt = std::vector<size_t>(omp_get_max_threads(),0);
  size_t cap = count;
  BAT* bn = COLnew(0, TYPE_oid, cap, TRANSIENT);
  oid* w = (oid*)(bn->theap.base);
  auto t_start = std::chrono::high_resolution_clock::now();
  BUN p = 0;
  fprintf(stderr, "bn->batCapacity = %lu\n", bn->batCapacity);


// #pragma omp parallel for
  for (size_t i = 0; i < count; i++ ){
    if ( column[i] >= lv && column[i] <= hv ) {
      // std::cout << "column["<<i<<"] = "<< (int)column[i] << " lv = " << (int)lv << " hv = " << (int)hv <<  std::endl;
      if ( p + 1 == bn->batCapacity ) {
        gdk_return suc = BATextend(bn, (bn->batCapacity)+1024*1024);
        // assert(suc==GDK_SUCCEED);
        cap=(bn->batCapacity)+1024*1024;
        w = (oid*)(bn->theap.base);
        fprintf(stderr, "realloced\n");
      }
      // fprintf(stderr, "p = %lu, bn->batCapacity = %lu, heap_free = %lu, heap_size = %lu\n",p, bn->batCapacity, bn->theap.free, bn->theap.size);
      w[p++] = i;
      // p++;
      // localcnt[omp_get_thread_num()]++;
      // localv[omp_get_thread_num()].push_back(i);
    }
  }
  BATsetcount(bn, p);
  
  auto t_end = std::chrono::high_resolution_clock::now();
  size_t size = sizeof(T)*count;
  double t_diff = std::chrono::duration<double, std::milli>(t_end-t_start).count();
  fprintf(stderr, "I am here!!(p=%lu) Throughput = (%luMB/%lfms)%lfMB/s\n", p,  size/1024/1024, t_diff, (double)size/1024/t_diff);
  // std::vector<size_t> retval;
  // for ( int i = 0; i < omp_get_max_threads(); i++){
  //   fprintf(stderr, "merge %dth vector\n", i);
  //   if ( localv[i].size() > 0 )
  //     retval.insert(retval.end(), localv[i].begin(), localv[i].end());
  // }
  // //free(localv);
  return bn;
}

template BAT* select(char* column, size_t count, char lv, char hv);
template BAT* select(int* column, size_t count, int lv, int hv);


gdk_return group(bte* column, BUN colsz, const BAT* s, BAT* g, BAT* e,
                 BAT** groups, BAT** extents, BAT** histo){

  const oid *grps = NULL;
  oid *restrict ngrps, ngrp, prev = 0, hseqb = 0;
  oid *restrict exts = NULL;
  lng *restrict cnts = NULL;

  BUN p, q, r;


  BUN maxgrps = g ? ((BUN) 1 << 8) * BATcount(e) : (BUN) 1 << 8;

  BUN start, end, cnt;
  const oid *restrict cand, *candend;

  start = 0;
  end = colsz;

  if ( s ){
	cand = (const oid *) Tloc((s), 0);
    candend = (const oid *) Tloc((s), BATcount(s));
  }
  else {
    cand = NULL;
    candend = NULL;
  }

  cnt = cand ? (BUN) (candend - cand) : end - start;

  fprintf(stderr, "group, cnt = %lu, s count = %lu\n", cnt, BATcount(s));
        
  BAT* gn = COLnew(0, TYPE_oid, cnt, TRANSIENT);
  if (gn == NULL) return GDK_FAIL;
  *groups = gn;
  ngrps = (oid *) Tloc(gn, 0);

  
  BAT* en = COLnew(0, TYPE_oid, maxgrps, TRANSIENT);
  if (en == NULL) return GDK_FAIL;
  *extents = en;
  exts = (oid *) Tloc(en, 0);

  
  BAT* hn = COLnew(0, TYPE_lng, maxgrps, TRANSIENT);
  if (hn == NULL) return GDK_FAIL;
  *histo = hn;
  cnts = (lng *) Tloc(hn, 0);
  memset(cnts, 0, maxgrps * sizeof(lng));

  if (g && (!BATordered(g) || !BATordered_rev(g)))
    grps = (const oid *) Tloc(g, 0);

  oid maxgrp = oid_nil;	/* maximum value of g BAT (if subgrouping) */
  PROPrec *prop;
  if (g) {
    if (BATtdense(g))
      maxgrp = g->tseqbase + BATcount(g);
    else if (BATtordered(g))
      maxgrp = * (oid *) Tloc(g, BATcount(g) - 1);
    else {
      prop = BATgetprop(g, GDK_MAX_VALUE);
      if (prop)
        maxgrp = prop->v.val.oval;
    }
    if (maxgrp == 0)
      g = NULL; /* single group */
  }

  const bte *w = (bte *) column;//Tloc(b, 0);
  if ( !grps ) {
    unsigned char *restrict bgrps =  (unsigned char *)GDKmalloc(256);
    unsigned char v;
    if (bgrps == NULL) return GDK_FAIL;
    memset(bgrps, 0xFF, 256);
  
    ngrp = 0;
    gn->tsorted = 1;
    r = 0;
    for (;;) {
      if (cand) {
        if (cand == candend)
          break;
        p = *cand++;
      } else {
        p = start++;
      }
      if (p >= end)
        break;
      if ((v = bgrps[w[p]]) == 0xFF && ngrp < 256) {
        fprintf(stderr, "new group v = %x, grpid = %lx\n", v, ngrp);
        bgrps[w[p]] = v = (unsigned char) ngrp++;
        if (extents)
          exts[v] = (oid) p;
      }
      ngrps[r] = v;
      if (r > 0 && v < ngrps[r - 1])
        gn->tsorted = 0;
      if (histo)
        cnts[v]++;
      r++;
    }

    BATsetcount(gn, r);
    BATsetcount(en, ngrp);
    BATsetcount(hn, ngrp);  
    GDKfree(bgrps);
  } else if ( maxgrps < 65536 ) {

    unsigned short *restrict sgrps = (unsigned short *)GDKmalloc(65536 * sizeof(short));
    unsigned short v;

    BUN probe;

    if (sgrps == NULL)
      goto error;
    memset(sgrps, 0xFF, 65536 * sizeof(short));
    
    ngrp = 0;
    gn->tsorted = 1;
    r = 0;
    for (;;) {
      if (cand) {
        if (cand == candend)
          break;
        p = *cand++;
      } else {
        p = start++;
      }
      if (p >= end)
        break;

      probe = (grps[r]<<8) | w[p];
      if ((v = sgrps[probe]) == 0xFFFF && ngrp < 65536) {
        fprintf(stderr, "new group v = %x, grpid = %lx, r=%lu, grps[r] = %lx, w[p] = %x \n", v, ngrp, r, grps[r], w[p]);
        sgrps[probe] = v = (unsigned short) ngrp++;
        if (extents)
          exts[v] =(oid) p;
      }
      ngrps[r] = v;
      if (r > 0 && v < ngrps[r - 1])
        gn->tsorted = 0;
      if (histo)
        cnts[v]++;
      r++;
    }
    GDKfree(sgrps);
    BATsetcount(gn, r);
    BATsetcount(en, ngrp);
    BATsetcount(hn, ngrp);  
  }
  else {
    //     if (grps && maxgrp != oid_nil
    // #if SIZEOF_OID == SIZEOF_LNG
    //         && maxgrp < ((oid) 1 << (SIZEOF_LNG * 8 - 8))
    // #endif
    //         )
    //       {

    fprintf(stderr, "supplied group in\n");
    char nme[20] = "grp_hashtable";
    size_t nmelen = strlen(nme);
    BUN mask = MAX(HASHmask(cnt), 1 << 16);
    BUN hb;      
    Heap* hp = (Heap*) GDKzalloc(sizeof(Heap));
    hp->farmid = BBPselectfarm(TRANSIENT, TYPE_bte, hashheap);
    hp->filename = (char*) GDKmalloc(nmelen + 30);
    snprintf(hp->filename, nmelen + 30,
             "%s.hash" SZFMT, nme, MT_getpid());


    Hash *hs = HASHnew(hp, TYPE_bte, s ? BATcount(s): colsz,
                       mask, BUN_NONE);

    BUN prb;
    oid grp;

    ulng v;

    fprintf(stderr, "creating partial hash table core....\n");

    GRP_create_partial_hash_table_core(
                                       (void) 0,
                                       (v = ((ulng)grps[r]<<8)|(unsigned char)w[p], hash_lng(hs, &v)),
                                       w[p] == w[hb] && grps[r] == grps[hb - start],
                                       (void) 0,
                                       NOGRPTST);

    fprintf(stderr, "done partial hash table core....\n");

      
    BATsetcount(gn, r);
    BATsetcount(en, ngrp);
    BATsetcount(hn, ngrp);  
    GDKfree(hp);
    GDKfree(hs);
    // } 

  }

  return GDK_SUCCEED;

 error:
  return GDK_FAIL;
  
}

template<typename TO, typename TI>
gdk_return aggr_sum(const TI* col, BUN colsz, const BAT* s, const BAT* g, const BAT* hist, BAT** result){
  if ( s )  assert(BATcount(s) == BATcount(g));
  else assert(BATcount(g) == colsz);

  BUN cnt = s ? BATcount(s) : colsz;

  BUN grpcnt = BATcount(hist);

  int tt;

  switch (sizeof(TO)) {
  case 1: tt = TYPE_bte; break;
  case 2: tt = TYPE_sht; break;
  case 4: tt = TYPE_int; break;
  case 8: tt = TYPE_lng; break;
  case 16: tt = TYPE_hge; fprintf(stderr, "tt = huge\n");break;
  }
 
  BAT* bn = COLnew(0, tt, grpcnt, TRANSIENT);
  BATsetcount(bn, grpcnt);
  
  *result = bn;

  TO* aggr = (TO*) Tloc(bn, 0);
  oid* grps = (oid*) Tloc(g, 0);
  memset(aggr, 0,  grpcnt* sizeof(TO));

  
  oid p;
  const oid* cand = s ? (oid*) Tloc(s,0) : NULL;

  fprintf(stderr, "aggr_sum, cnt = %lu, grpcnt = %lu\n", cnt, grpcnt);
  for ( oid i = 0; i < cnt; i++ ){
    p = s ? cand[i] : i ;
    TI val = col[p];
    oid grp = grps[i];
    aggr[grp] += ((TO) val);
  }



  return GDK_SUCCEED;

}

template gdk_return aggr_sum<long long, int>(const int* col, BUN colsz, const BAT* s, const BAT* g, const BAT* hist, BAT** result);
template gdk_return aggr_sum<long long, long long>(const long long* col, BUN colsz, const BAT* s, const BAT* g, const BAT* hist, BAT** result);
template gdk_return aggr_sum<hge, lng>(const lng* col, BUN colsz, const BAT* s, const BAT* g, const BAT* hist, BAT** result);

// template gdk_return aggr_sum(const long long* col, BUN colsz, const BAT* s, const BAT* g, const BAT* hist, BAT** result);


template<typename T>
BAT* merge(const T* col1,  BUN colsz1, BAT* s1, const T* col2, BUN colsz2, BAT* s2, std::function<T(T,T)> mergefunc){
  BUN cnt1 = s1 ? BATcount(s1) : colsz1;
  BUN cnt2 = s2 ? BATcount(s2) : colsz2;
  assert(cnt1 == cnt2);
  BUN cnt = cnt1;
  int tt;

  switch (sizeof(T)) {
  case 1: tt = TYPE_bte; break;
  case 2: tt = TYPE_sht; break;
  case 4: tt = TYPE_int; break;
  case 8: tt = TYPE_lng; break;
  case 16: tt = TYPE_hge; break;
  }
 

  BAT* bn = COLnew(0, tt, cnt, TRANSIENT);
  BATsetcount(bn, cnt);
  T* outp = (T*)Tloc(bn,0);


  // T* w1 = (T*) col1;
  // T* w2 = (T*) col2;
  
  oid p1, p2;
  const oid* cand1 = s1 ? (oid*) Tloc(s1,0) : NULL;
  const oid* cand2 = s2 ? (oid*) Tloc(s2,0) : NULL;

  // fprintf(stderr, "aggr_sum, cnt = %lu, grpcnt = %lu\n", cnt, grpcnt);
  for ( oid i = 0; i < cnt; i++ ){
    p1 = s1 ? cand1[i] : i ;
    p2 = s2 ? cand2[i] : i ;
    outp[i] = mergefunc(col1[p1], col2[p2]);
  }

  return bn;

}


template BAT* merge<lng>(const lng* col1,  BUN colsz1, BAT* s1, const lng* col2, BUN colsz2, BAT* s2, std::function<lng(lng,lng)> mergefunc);

template BAT* merge<ulng>(const ulng* col1,  BUN colsz1, BAT* s1, const ulng* col2, BUN colsz2, BAT* s2, std::function<ulng(ulng,ulng)> mergefunc);

// template BAT* merge<ulng>(const ulng* col1,  BUN colsz1, BAT* s1, const ulng* col2, BUN colsz2, BAT* s2, std::function<ulng(ulng,ulng)> mergefunc);